#include "pluginrepository.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QJsonObject>
#include <QPluginLoader>

#include "oimetadata.h"
#include "pluginabi.h"
#include "pluginloader.h"
#include "settings.h"
#include "util.h"

QList<sys::Plugin> PluginRepository::plugins;
QStringList PluginRepository::rejected;
bool PluginRepository::isInit = false;

/*!
 * \brief PluginRepository::init
 * Scans both the system tier (<appDir>/plugins/, installed with the app)
 * and the user tier (~/.openindy/plugins/, always writable without
 * elevated permissions - where PluginCopier's import flow writes to) once,
 * caching the merged result for the session. A user-tier plugin sharing a
 * system-tier one's name shadows it, so a regular user can update/override
 * a plugin without touching the install directory.
 */
void PluginRepository::init() {
  QString systemPluginDir = qApp->applicationDirPath() + "/plugins";
  QString userPluginDir = QDir::homePath() + "/.openindy/plugins";

  QList<sys::Plugin> systemPlugins =
      PluginRepository::scanDirectory(systemPluginDir);
  QList<sys::Plugin> userPlugins =
      PluginRepository::scanDirectory(userPluginDir);

  QList<sys::Plugin> merged = systemPlugins;
  foreach (const sys::Plugin& userPlugin, userPlugins) {
    bool shadowed = false;
    for (int i = 0; i < merged.size(); i++) {
      if (merged.at(i).name().compare(userPlugin.name()) == 0) {
        qWarning() << "PluginRepository: user plugin" << userPlugin.filePath()
                   << "shadows system plugin" << merged.at(i).filePath()
                   << "(both named" << userPlugin.name() << ")";
        merged[i] = userPlugin;
        shadowed = true;
        break;
      }
    }
    if (!shadowed) {
      merged.append(userPlugin);
    }
  }

  PluginRepository::plugins = merged;
  PluginRepository::isInit = true;
}

/*!
 * \brief PluginRepository::scanDirectory
 * \param dirPath
 * \return every valid OpenIndy plugin found directly in dirPath
 */
QList<sys::Plugin> PluginRepository::scanDirectory(const QString& dirPath) {
  QList<sys::Plugin> result;

  QDir dir(dirPath);
  if (!dir.exists()) {
    return result;
  }

  foreach (
      const QString& fileName,
      dir.entryList(QStringList() << "p_*.dll" << "libp_*.so", QDir::Files)) {
    QString filePath = dir.absoluteFilePath(fileName);

    QPluginLoader probe(filePath);
    QJsonObject meta = probe.metaData().value("MetaData").toObject();
    if (!meta.value("isOiPlugin").toBool()) {
      continue;
    }

    // An ABI mismatch used to be indistinguishable from "not an OpenIndy
    // plugin": qobject_cast<Plugin*> simply returned null and the driver
    // vanished with no message. It is checked here, from metadata, before
    // the library is ever instantiated - so the diagnosis names the file,
    // both versions, and the fix.
    if (!PluginRepository::checkAbiVersion(filePath, meta)) {
      continue;
    }

    result.append(PluginRepository::scanPluginFile(filePath));
  }

  return result;
}

/*!
 * \brief PluginRepository::checkAbiVersion
 * \param filePath
 * \param meta the plugin's "MetaData" object, read without loading the library
 * \return true if this plugin was built against the ABI this app speaks
 */
bool PluginRepository::checkAbiVersion(const QString& filePath,
                                       const QJsonObject& meta) {
  const QJsonValue declared = meta.value("abiVersion");

  if (!declared.isDouble()) {
    // predates abiVersion entirely, so it cannot be checked and cannot be
    // assumed compatible - loading it risks a vtable mismatch at runtime,
    // which is far worse than not loading it
    const QString reason = QString(
                               "%1: no abiVersion in its metadata (built "
                               "against a core older than "
                               "plugin ABI %2). Rebuild the plugin.")
                               .arg(filePath)
                               .arg(OI_PLUGIN_ABI_VERSION);
    qWarning().noquote() << "PluginRepository: rejected" << reason;
    PluginRepository::rejected.append(reason);
    return false;
  }

  const int pluginAbi = declared.toInt();
  if (pluginAbi != OI_PLUGIN_ABI_VERSION) {
    const QString reason = QString(
                               "%1: built against plugin ABI %2, this build of "
                               "OpenIndy requires %3. "
                               "Rebuild the plugin against the current core.")
                               .arg(filePath)
                               .arg(pluginAbi)
                               .arg(OI_PLUGIN_ABI_VERSION);
    qWarning().noquote() << "PluginRepository: rejected" << reason;
    PluginRepository::rejected.append(reason);
    return false;
  }

  return true;
}

/*!
 * \brief PluginRepository::getRejectedPlugins
 * \return one human-readable line per plugin skipped for ABI reasons, so the
 * UI can show "3 plugins were not loaded" rather than leaving the user to
 * wonder where their instrument went
 */
const QStringList& PluginRepository::getRejectedPlugins() {
  if (!PluginRepository::isInit) {
    PluginRepository::init();
  }
  return PluginRepository::rejected;
}

/*!
 * \brief PluginRepository::scanPluginFile
 * Instantiates every Function/Sensor/Tool/Exchange/Simulation a plugin file
 * offers and calls init() on each - the same work PluginCopier::savePlugin()
 * used to do once at import time to populate the DB, now done at startup.
 * \param filePath
 * \return
 */
sys::Plugin PluginRepository::scanPluginFile(const QString& filePath) {
  QPluginLoader pluginLoader(filePath);
  QJsonObject meta = pluginLoader.metaData().value("MetaData").toObject();

  QString name = meta.value("name").toString();
  QString description = meta.value("description").toString();
  QString version = meta.value("pluginVersion").toString();
  QString author = meta.value("author").toString();

  // lightweight owner reference embedded into each function/sensor/etc below -
  // empty child lists, since a function shouldn't carry a copy of its plugin's
  // entire other contents
  sys::Plugin ownerRef(name, description, version, author, filePath,
                       QList<sys::Function>(), QList<sys::Sensor>(),
                       QList<sys::Tool>(), QList<sys::Exchange>(),
                       QList<sys::Simulation>());

  QList<sys::Function> functions;
  foreach (const QPointer<Function>& f,
           PluginLoader::loadFunctionPlugins(filePath)) {
    if (f.isNull()) {
      continue;
    }
    QList<ElementTypes> neededElements;
    foreach (const NeededElement& ne, f->getNeededElements()) {
      neededElements.append(ne.typeOfElement);
    }
    functions.append(sys::Function(
        f->getMetaData().name, f->getMetaData().description, ownerRef,
        f->getApplicableFor(), neededElements, f->getMetaData().iid));
  }

  QList<sys::Sensor> sensors;
  foreach (const QPointer<Sensor>& s,
           PluginLoader::loadSensorPlugins(filePath)) {
    if (s.isNull()) {
      continue;
    }

    // tag every config this sensor publishes with its own type here, once,
    // rather than asking each driver to derive and stamp its own SensorType -
    // the derivation already lives in one place (SensorType::fromInterfaceId)
    const SensorType type = SensorType::fromInterfaceId(s->getMetaData().iid);
    QList<MeasurementConfig> configs = s->getSupportedMeasurementConfigs();
    for (int i = 0; i < configs.size(); i++) {
      configs[i].setSensorType(type);
    }

    sensors.append(sys::Sensor(
        s->getMetaData().name, s->getMetaData().description, ownerRef,
        s->getMetaData().iid, configs, s->getSupportedAcquisitionModes()));
  }

  QList<sys::Tool> tools;
  foreach (const QPointer<Tool>& t, PluginLoader::loadToolPlugins(filePath)) {
    if (t.isNull()) {
      continue;
    }
    tools.append(sys::Tool(t->getMetaData().name, t->getMetaData().description,
                           ownerRef));
  }

  QList<sys::Exchange> exchanges;
  foreach (const QPointer<ExchangeSimpleAscii>& e,
           PluginLoader::loadExchangeSimpleAsciiPlugins(filePath)) {
    if (e.isNull()) {
      continue;
    }
    exchanges.append(sys::Exchange(e->getMetaData().name,
                                   e->getMetaData().description, ownerRef,
                                   e->getMetaData().iid));
  }
  foreach (const QPointer<ExchangeDefinedFormat>& e,
           PluginLoader::loadExchangeDefinedFormatPlugins(filePath)) {
    if (e.isNull()) {
      continue;
    }
    exchanges.append(sys::Exchange(e->getMetaData().name,
                                   e->getMetaData().description, ownerRef,
                                   e->getMetaData().iid));
  }

  QList<sys::Simulation> simulations;
  foreach (const QPointer<SimulationModel>& sim,
           PluginLoader::loadSimulationPlugins(filePath)) {
    if (sim.isNull()) {
      continue;
    }
    simulations.append(sys::Simulation(
        sim->getMetaData().name, sim->getMetaData().description, ownerRef));
  }

  return sys::Plugin(name, description, version, author, filePath, functions,
                     sensors, tools, exchanges, simulations);
}

/*!
 * \brief PluginRepository::getAll
 * \return
 */
QList<sys::Plugin> PluginRepository::getAll() {
  if (!PluginRepository::isInit) {
    PluginRepository::init();
  }
  return PluginRepository::plugins;
}

/*!
 * \brief PluginRepository::getByName
 * Convenience/display lookup - first match wins if two plugins share a name.
 * \param name
 * \return
 */
std::optional<sys::Plugin> PluginRepository::getByName(const QString& name) {
  foreach (const sys::Plugin& plugin, PluginRepository::getAll()) {
    if (plugin.name().compare(name) == 0) {
      return plugin;
    }
  }
  return std::nullopt;
}

/*!
 * \brief PluginRepository::getByFilePath
 * The real, collision-safe lookup - file paths can't collide.
 * \param filePath
 * \return
 */
std::optional<sys::Plugin> PluginRepository::getByFilePath(
    const QString& filePath) {
  foreach (const sys::Plugin& plugin, PluginRepository::getAll()) {
    if (plugin.filePath().compare(filePath) == 0) {
      return plugin;
    }
  }
  return std::nullopt;
}

/*!
 * \brief PluginRepository::getAllFunctions
 * \return every function across every loaded plugin
 */
QList<sys::Function> PluginRepository::getAllFunctions() {
  QList<sys::Function> result;
  foreach (const sys::Plugin& plugin, PluginRepository::getAll()) {
    result.append(plugin.functions());
  }
  return result;
}

/*!
 * \brief PluginRepository::getFunctionsApplicableTo
 * Functions used to CREATE a feature of the given type (fit/construct/special,
 * or system-transformation for trafo params) - mirrors SystemDbManager's old
 * getCreateFunctions() category split.
 * \param type
 * \return
 */
bool PluginRepository::isCreateFunctionFor(const sys::Function& function,
                                           const FeatureTypes& type) {
  bool isCreateCategory;
  if (type == eTrafoParamFeature) {
    isCreateCategory = function.interfaceCategory().compare(
                           OiMetaData::iid_SystemTransformation) == 0;
  } else {
    isCreateCategory =
        function.interfaceCategory().compare(OiMetaData::iid_FitFunction) ==
            0 ||
        function.interfaceCategory().compare(
            OiMetaData::iid_ConstructFunction) == 0 ||
        function.interfaceCategory().compare(OiMetaData::iid_SpecialFunction) ==
            0;
  }
  return isCreateCategory && function.applicableFor().contains(type);
}

QList<sys::Function> PluginRepository::getFunctionsApplicableTo(
    const FeatureTypes& type) {
  QList<sys::Function> result;
  foreach (const sys::Function& function, PluginRepository::getAllFunctions()) {
    if (PluginRepository::isCreateFunctionFor(function, type)) {
      result.append(function);
    }
  }
  return result;
}

/*!
 * \brief PluginRepository::getChangeFunctions
 * Functions used to CHANGE an already-solved feature of the given type -
 * everything that isn't a create-category function, per SystemDbManager's
 * old getChangeFunctions() split.
 * \param type
 * \return
 */
QList<sys::Function> PluginRepository::getChangeFunctions(
    const FeatureTypes& type) {
  QList<sys::Function> result;
  foreach (const sys::Function& function, PluginRepository::getAllFunctions()) {
    bool isCreateCategory =
        function.interfaceCategory().compare(
            OiMetaData::iid_SystemTransformation) == 0 ||
        function.interfaceCategory().compare(OiMetaData::iid_FitFunction) ==
            0 ||
        function.interfaceCategory().compare(
            OiMetaData::iid_ConstructFunction) == 0 ||
        function.interfaceCategory().compare(OiMetaData::iid_SpecialFunction) ==
            0;
    if (!isCreateCategory && function.applicableFor().contains(type)) {
      result.append(function);
    }
  }
  return result;
}

/*!
 * \brief PluginRepository::getByQualifiedId
 * \param qualifiedId
 * \return
 */
std::optional<sys::Function> PluginRepository::getByQualifiedId(
    const QString& qualifiedId) {
  foreach (const sys::Function& function, PluginRepository::getAllFunctions()) {
    if (function.qualifiedId().compare(qualifiedId) == 0) {
      return function;
    }
  }
  return std::nullopt;
}

/*!
 * \brief PluginRepository::getAllTools
 * \return
 */
QList<sys::Tool> PluginRepository::getAllTools() {
  QList<sys::Tool> result;
  foreach (const sys::Plugin& plugin, PluginRepository::getAll()) {
    result.append(plugin.tools());
  }
  return result;
}

/*!
 * \brief PluginRepository::getAllSensors
 * \return
 */
QList<sys::Sensor> PluginRepository::getAllSensors() {
  QList<sys::Sensor> result;
  foreach (const sys::Plugin& plugin, PluginRepository::getAll()) {
    result.append(plugin.sensors());
  }
  return result;
}

/*!
 * \brief PluginRepository::getAllMeasurementConfigs
 * Every config every scanned sensor publishes, tagged with that sensor's
 * type - the driver-published tier MeasurementConfigManager merges with
 * the user/project tier. Available without any sensor being connected,
 * since it comes from the same instantiate+init() scan that already
 * builds the plugin catalog.
 */
QList<MeasurementConfig> PluginRepository::getAllMeasurementConfigs() {
  QList<MeasurementConfig> result;
  foreach (const sys::Sensor& sensor, PluginRepository::getAllSensors()) {
    result.append(sensor.measurementConfigs());
  }
  return result;
}

/*!
 * \brief PluginRepository::getDefaultFunction
 * Resolves Settings' "defaultFunction.<type>" key, most specific tier first,
 * skipping any tier whose reference doesn't actually resolve to a real
 * applicable function (a stale reference must not come back as a result).
 * \param type
 * \return
 */
std::optional<sys::Function> PluginRepository::getDefaultFunction(
    const FeatureTypes& type) {
  QString key = QString("defaultFunction.%1").arg(getFeatureTypeName(type));
  QString name = Settings::get(key);
  if (name.isEmpty()) {
    return std::nullopt;
  }

  foreach (const sys::Function& function,
           PluginRepository::getFunctionsApplicableTo(type)) {
    if (function.name().compare(name) == 0) {
      return function;
    }
  }

  return std::nullopt;
}

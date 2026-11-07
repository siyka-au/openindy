#include "pluginrepository.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QJsonObject>
#include <vector>

#include "containers.h"
#include "oimetadata.h"
#include "pluginhost.h"
#include "pluginmanager.h"
#include "qtconvert.h"
#include "settings.h"
#include "util.h"

std::vector<sys::Plugin> PluginRepository::plugins;
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

  std::vector<sys::Plugin> systemPlugins =
      PluginRepository::scanDirectory(systemPluginDir);
  std::vector<sys::Plugin> userPlugins =
      PluginRepository::scanDirectory(userPluginDir);

  std::vector<sys::Plugin> merged = systemPlugins;
  foreach (const sys::Plugin& userPlugin, userPlugins) {
    bool shadowed = false;
    for (auto& i : merged) {
      if (i.name().compare(userPlugin.name()) == 0) {
        qWarning() << "PluginRepository: user plugin" << userPlugin.filePath()
                   << "shadows system plugin" << i.filePath() << "(both named"
                   << userPlugin.name() << ")";
        i = userPlugin;
        shadowed = true;
        break;
      }
    }
    if (!shadowed) {
      merged.push_back(userPlugin);
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
std::vector<sys::Plugin> PluginRepository::scanDirectory(
    const QString& dirPath) {
  std::vector<sys::Plugin> result;

  QDir dir(dirPath);
  if (!dir.exists()) {
    return result;
  }

  foreach (
      const QString& fileName,
      dir.entryList(QStringList() << "p_*.dll" << "libp_*.so", QDir::Files)) {
    QString filePath = dir.absoluteFilePath(fileName);

    const PluginProbe probe = PluginHost::probe(filePath);
    if (probe.status == oi::PluginLoadStatus::kAbiMismatch) {
      // An ABI mismatch used to be indistinguishable from "not an OpenIndy
      // plugin": the driver simply vanished with no message. The core loader
      // checks the version before the factory is ever created, and its error
      // names the file, both versions and the fix.
      qWarning().noquote() << "PluginRepository: rejected" << probe.error;
      PluginRepository::rejected.append(probe.error);
      continue;
    }
    if (!probe.isOiPlugin) {
      continue;
    }

    result.push_back(PluginRepository::scanPluginFile(filePath));
  }

  return result;
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
 * \brief loadIf
 * A library that only carries UI tools has no compute factory to ask.
 */
template <typename Result>
static Result loadIf(bool compute, Result (*loader)(const QString&),
                     const QString& path) {
  return compute ? loader(path) : Result();
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
  const PluginProbe probe = PluginHost::probe(filePath);
  const QJsonObject& meta = probe.meta;

  QString name = meta.value("name").toString();
  QString description = meta.value("description").toString();
  QString version = meta.value("pluginVersion").toString();
  QString author = meta.value("author").toString();

  // lightweight owner reference embedded into each function/sensor/etc below -
  // empty child lists, since a function shouldn't carry a copy of its plugin's
  // entire other contents
  sys::Plugin ownerRef(name, description, version, author, filePath,
                       std::vector<sys::Function>(), std::vector<sys::Sensor>(),
                       std::vector<sys::Tool>(), std::vector<sys::Exchange>(),
                       std::vector<sys::Simulation>());

  std::vector<sys::Function> functions;
  foreach (const oi::Guarded<Function>& f,
           loadIf(probe.plugin != nullptr, &PluginManager::loadFunctionPlugins,
                  filePath)) {
    if (f.isNull()) {
      continue;
    }
    std::vector<ElementTypes> neededElements;
    foreach (const NeededElement& ne, f->getNeededElements()) {
      neededElements.push_back(ne.typeOfElement);
    }
    functions.push_back(sys::Function(toQString(f->getMetaData().name),
                                      toQString(f->getMetaData().description),
                                      ownerRef, f->getApplicableFor(),
                                      neededElements,
                                      toQString(f->getMetaData().iid)));
  }

  std::vector<sys::Sensor> sensors;
  foreach (const oi::Guarded<Sensor>& s,
           loadIf(probe.plugin != nullptr, &PluginManager::loadSensorPlugins,
                  filePath)) {
    if (s.isNull()) {
      continue;
    }

    // tag every config this sensor publishes with its own type here, once,
    // rather than asking each driver to derive and stamp its own SensorType -
    // the derivation already lives in one place (SensorType::fromInterfaceId)
    const SensorType type =
        SensorType::fromInterfaceId(toStd(toQString(s->getMetaData().iid)));
    std::vector<MeasurementConfig> configs =
        s->getSupportedMeasurementConfigs();
    for (auto& config : configs) {
      config.setSensorType(type);
    }

    sensors.push_back(sys::Sensor(toQString(s->getMetaData().name),
                                  toQString(s->getMetaData().description),
                                  ownerRef, toQString(s->getMetaData().iid),
                                  configs, s->getSupportedAcquisitionModes()));
  }

  std::vector<sys::Tool> tools;
  foreach (const QPointer<Tool>& t, PluginManager::loadToolPlugins(filePath)) {
    if (t.isNull()) {
      continue;
    }
    tools.push_back(sys::Tool(toQString(t->getMetaData().name),
                              toQString(t->getMetaData().description),
                              ownerRef));
  }

  std::vector<sys::Exchange> exchanges;
  foreach (const oi::Guarded<ExchangeSimpleAscii>& e,
           loadIf(probe.plugin != nullptr,
                  &PluginManager::loadExchangeSimpleAsciiPlugins, filePath)) {
    if (e.isNull()) {
      continue;
    }
    exchanges.push_back(sys::Exchange(toQString(e->getMetaData().name),
                                      toQString(e->getMetaData().description),
                                      ownerRef,
                                      toQString(e->getMetaData().iid)));
  }
  foreach (const oi::Guarded<ExchangeDefinedFormat>& e,
           loadIf(probe.plugin != nullptr,
                  &PluginManager::loadExchangeDefinedFormatPlugins, filePath)) {
    if (e.isNull()) {
      continue;
    }
    exchanges.push_back(sys::Exchange(toQString(e->getMetaData().name),
                                      toQString(e->getMetaData().description),
                                      ownerRef,
                                      toQString(e->getMetaData().iid)));
  }

  std::vector<sys::Simulation> simulations;
  foreach (const oi::Guarded<SimulationModel>& sim,
           loadIf(probe.plugin != nullptr,
                  &PluginManager::loadSimulationPlugins, filePath)) {
    if (sim.isNull()) {
      continue;
    }
    simulations.push_back(
        sys::Simulation(toQString(sim->getMetaData().name),
                        toQString(sim->getMetaData().description), ownerRef));
  }

  return sys::Plugin(name, description, version, author, filePath, functions,
                     sensors, tools, exchanges, simulations);
}

/*!
 * \brief PluginRepository::getAll
 * \return
 */
std::vector<sys::Plugin> PluginRepository::getAll() {
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
std::vector<sys::Function> PluginRepository::getAllFunctions() {
  std::vector<sys::Function> result;
  foreach (const sys::Plugin& plugin, PluginRepository::getAll()) {
    oi::appendAll(result, plugin.functions());
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
  bool isCreateCategory = false;
  if (type == FeatureTypes::eTrafoParamFeature) {
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
  return isCreateCategory && oi::contains(function.applicableFor(), type);
}

std::vector<sys::Function> PluginRepository::getFunctionsApplicableTo(
    const FeatureTypes& type) {
  std::vector<sys::Function> result;
  foreach (const sys::Function& function, PluginRepository::getAllFunctions()) {
    if (PluginRepository::isCreateFunctionFor(function, type)) {
      result.push_back(function);
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
std::vector<sys::Function> PluginRepository::getChangeFunctions(
    const FeatureTypes& type) {
  std::vector<sys::Function> result;
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
    if (!isCreateCategory && oi::contains(function.applicableFor(), type)) {
      result.push_back(function);
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
std::vector<sys::Tool> PluginRepository::getAllTools() {
  std::vector<sys::Tool> result;
  foreach (const sys::Plugin& plugin, PluginRepository::getAll()) {
    oi::appendAll(result, plugin.tools());
  }
  return result;
}

/*!
 * \brief PluginRepository::getAllSensors
 * \return
 */
std::vector<sys::Sensor> PluginRepository::getAllSensors() {
  std::vector<sys::Sensor> result;
  foreach (const sys::Plugin& plugin, PluginRepository::getAll()) {
    oi::appendAll(result, plugin.sensors());
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
std::vector<MeasurementConfig> PluginRepository::getAllMeasurementConfigs() {
  std::vector<MeasurementConfig> result;
  foreach (const sys::Sensor& sensor, PluginRepository::getAllSensors()) {
    oi::appendAll(result, sensor.measurementConfigs());
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

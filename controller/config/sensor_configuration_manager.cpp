#include "sensor_configuration_manager.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <vector>

#include "config_file.h"
#include "containers.h"
#include "qt_convert.h"

namespace {
constexpr int kAccuracyCompareDecimals =
    8;  // decimals used when comparing sigma values
}  // namespace

/*!
 * \brief SensorConfigurationManager::SensorConfigurationManager
 */
SensorConfigurationManager::SensorConfigurationManager(QObject* parent)
    : QObject(parent) {}

/*!
 * \brief SensorConfigurationManager::SensorConfigurationManager
 * \param copy
 * \param parent
 */
SensorConfigurationManager::SensorConfigurationManager(
    const SensorConfigurationManager& copy, QObject* parent)
    : QObject(parent),
      activeSensorConfig(copy.activeSensorConfig),
      projectSensorConfigsList(copy.projectSensorConfigsList),
      projectSensorConfigsMap(copy.projectSensorConfigsMap),
      savedSensorConfigsList(copy.savedSensorConfigsList),
      savedSensorConfigsMap(copy.savedSensorConfigsMap) {}

/*!
 * \brief SensorConfigurationManager::operator =
 * \param copy
 * \return
 */
SensorConfigurationManager& SensorConfigurationManager::operator=(
    const SensorConfigurationManager& copy) {
  this->savedSensorConfigsMap = copy.savedSensorConfigsMap;
  this->savedSensorConfigsList = copy.savedSensorConfigsList;
  this->projectSensorConfigsMap = copy.projectSensorConfigsMap;
  this->projectSensorConfigsList = copy.projectSensorConfigsList;
  this->activeSensorConfig = copy.activeSensorConfig;
  return *this;
}

/*!
 * \brief SensorConfigurationManager::hasSavedSensorConfig
 * Checks wether there is a saved sensor config with the given name
 * \param name
 * \return
 */
bool SensorConfigurationManager::hasSavedSensorConfig(const QString& name) {
  return this->savedSensorConfigsMap.contains(toStd(name));
}

/*!
 * \brief SensorConfigurationManager::hasProjectSensorConfig
 * Checks wether there is a project sensor config with the given name
 * \param name
 * \return
 */
bool SensorConfigurationManager::hasProjectSensorConfig(const QString& name) {
  return this->projectSensorConfigsMap.contains(toStd(name));
}

/*!
 * \brief SensorConfigurationManager::hasSavedSensorConfig
 * Checks wether there is a saved sensor config with the same name and
 * parameters
 * \param sConfig
 * \return
 */
bool SensorConfigurationManager::hasSavedSensorConfig(
    const SensorConfiguration& sConfig) {
  if (!this->savedSensorConfigsMap.contains(sConfig.getName())) {
    return false;
  }

  // get saved config and compare it to the given one
  SensorConfiguration userConfig =
      oi::value(this->savedSensorConfigsMap, sConfig.getName());

  return this->equals(sConfig, userConfig);
}

/*!
 * \brief SensorConfigurationManager::hasProjectSensorConfig
 * Checks wether there is a project sensor config with the same name and
 * parameters
 * \param sConfig
 * \return
 */
bool SensorConfigurationManager::hasProjectSensorConfig(
    const SensorConfiguration& sConfig) {
  if (!this->projectSensorConfigsMap.contains(sConfig.getName())) {
    return false;
  }

  // get project config and compare it to the given one
  SensorConfiguration projectConfig =
      oi::value(this->projectSensorConfigsMap, sConfig.getName());

  return this->equals(sConfig, projectConfig);
}

/*!
 * \brief SensorConfigurationManager::getSensorConfig
 * Returns the sensor config with the specified name
 * \param name
 * \return
 */
SensorConfiguration SensorConfigurationManager::getSavedSensorConfig(
    const QString& name) const {
  return oi::value(this->savedSensorConfigsMap, toStd(name),
                   SensorConfiguration());
}

/*!
 * \brief SensorConfigurationManager::getProjectSensorConfig
 * \param name
 * \return
 */
SensorConfiguration SensorConfigurationManager::getProjectSensorConfig(
    const QString& name) const {
  return oi::value(this->projectSensorConfigsMap, toStd(name),
                   SensorConfiguration());
}

/*!
 * \brief SensorConfigurationManager::getSavedSensorConfigs
 * \return
 */
const std::vector<SensorConfiguration>&
SensorConfigurationManager::getSavedSensorConfigs() const {
  return this->savedSensorConfigsList;
}

/*!
 * \brief SensorConfigurationManager::getProjectSensorConfigs
 * \return
 */
const std::vector<SensorConfiguration>&
SensorConfigurationManager::getProjectSensorConfigs() const {
  return this->projectSensorConfigsList;
}

/*!
 * \brief SensorConfigurationManager::getActiveSensorConfig
 * Returns the active sensor config
 * \return
 */
const SensorConfiguration& SensorConfigurationManager::getActiveSensorConfig()
    const {
  return this->activeSensorConfig;
}

/*!
 * \brief SensorConfigurationManager::setActiveSensorConfig
 * \param sConfig
 */
void SensorConfigurationManager::setActiveSensorConfig(
    const SensorConfiguration& sConfig) {
  this->activeSensorConfig = sConfig;
  emit this->activeSensorConfigurationChanged();
}

/*!
 * \brief SensorConfigurationManager::addSavedSensorConfig
 * \param sConfig
 * \param save
 */
void SensorConfigurationManager::addSavedSensorConfig(
    const SensorConfiguration& sConfig, bool save) {
  // check if sConfig is valid
  if (!sConfig.getIsValid()) {
    emit this->sendMessage(
        "Cannot add a sensor configuration with an empty name",
        MessageTypes::eErrorMessage);
    return;
  }

  // check if sConfig already exists
  if (this->savedSensorConfigsMap.contains(sConfig.getName())) {
    emit this->sendMessage(
        QString("A sensor configuration with the name %1 already exists")
            .arg(sConfig.getName()),
        MessageTypes::eErrorMessage);
    return;
  }

  // save sConfig
  this->saveSensorConfig(sConfig, save);
}

/*!
 * \brief SensorConfigurationManager::addProjectSensorConfig
 * \param sConfig
 */
void SensorConfigurationManager::addProjectSensorConfig(
    const SensorConfiguration& sConfig) {
  // check if sConfig is valid
  if (!sConfig.getIsValid()) {
    emit this->sendMessage(
        "Cannot add a sensor configuration with an empty name",
        MessageTypes::eErrorMessage);
    return;
  }

  // check if sConfig already exists
  if (this->projectSensorConfigsMap.contains(sConfig.getName())) {
    emit this->sendMessage(
        QString("A sensor configuration with the name %1 already exists")
            .arg(sConfig.getName()),
        MessageTypes::eErrorMessage);
    return;
  }

  // save sConfig
  this->projectSensorConfigsList.push_back(sConfig);
  this->projectSensorConfigsMap.insert_or_assign(sConfig.getName(), sConfig);
}

/*!
 * \brief SensorConfigurationManager::removeSavedSensorConfig
 * \param name
 * \param save
 */
void SensorConfigurationManager::removeSavedSensorConfig(const QString& name,
                                                         bool save) {
  // check name
  if (name.compare("") == 0) {
    emit this->sendMessage(
        "Cannot remove a sensor configuration with an empty name",
        MessageTypes::eErrorMessage);
    return;
  }

  // check if the sensor config exists
  if (!this->savedSensorConfigsMap.contains(toStd(name))) {
    emit this->sendMessage(
        QString("A sensor configuration with the name %1 does not exist")
            .arg(name),
        MessageTypes::eErrorMessage);
    return;
  }

  // delete sConfig
  this->deleteSensorConfig(name, save);
}

/*!
 * \brief SensorConfigurationManager::removeProjectSensorConfig
 * \param name
 */
void SensorConfigurationManager::removeProjectSensorConfig(
    const QString& name) {
  // check if the sensor config exists
  if (!this->projectSensorConfigsMap.contains(toStd(name))) {
    emit this->sendMessage(
        QString("A sensor configuration with the name %1 does not exist")
            .arg(name),
        MessageTypes::eErrorMessage);
    return;
  }

  // remove sensor config
  SensorConfiguration sConfig =
      oi::value(this->projectSensorConfigsMap, toStd(name));
  this->projectSensorConfigsMap.erase(toStd(name));
  oi::removeOne(this->projectSensorConfigsList, sConfig);
}

/*!
 * \brief SensorConfigurationManager::removeAllSensorConfigs
 */
void SensorConfigurationManager::removeAllSavedSensorConfigs(bool save) {
  // get a list of saved sensor configs
  std::vector<SensorConfiguration> configs = this->getSavedSensorConfigs();

  // remove sensor configs
  foreach (const SensorConfiguration& sConfig, configs) {
    this->deleteSensorConfig(toQString(sConfig.getName()), save);
  }
}

/*!
 * \brief SensorConfigurationManager::removeAllProjectSensorConfigs
 */
void SensorConfigurationManager::removeAllProjectSensorConfigs() {
  this->projectSensorConfigsList.clear();
  this->projectSensorConfigsMap.clear();

  emit this->sensorConfigurationsChanged();
}

/*!
 * \brief SensorConfigurationManager::replaceSensorConfig
 * Replaces the sensor config with the name name by the given sensor config
 * \param name
 * \param sConfig
 * \param save
 */
void SensorConfigurationManager::replaceSensorConfig(
    const QString& name, const SensorConfiguration& sConfig, bool save) {
  // get the old sensor config
  if (!this->savedSensorConfigsMap.contains(toStd(name))) {
    return;
  }
  SensorConfiguration oldConfig =
      oi::value(this->savedSensorConfigsMap, toStd(name));

  // check active config
  bool isActive = false;
  if (this->activeSensorConfig.getIsSaved() &&
      this->activeSensorConfig.getName().compare(toStd(name)) == 0) {
    isActive = true;
  }

  // ########################
  // replace sConfig json file
  // ########################

  if (save) {
    // get config folder (create if does not exist yet)
    QString appPath = qApp->applicationDirPath();
    QDir sConfigFolder(appPath.append("/config/sensorConfigs"));
    if (!sConfigFolder.exists()) {
      sConfigFolder.mkpath(".");
    }

    // remove old config file
    QFile oldConfigFile(sConfigFolder.absoluteFilePath(name + ".json"));
    if (oldConfigFile.exists()) {
      oldConfigFile.remove();
    }

    // save sConfig to json file
    configfile::write(
        sConfigFolder.absoluteFilePath(toQString(sConfig.getName() + ".json")),
        oi::codec::encodeSensorConfiguration(sConfig));
  }

  // ###############################
  // replace sConfig in list and map
  // ###############################

  // replace sConfig in map
  this->savedSensorConfigsMap.erase(toStd(name));
  this->savedSensorConfigsMap.insert_or_assign(sConfig.getName(), sConfig);

  // replace sConfig in list
  int index = oi::indexOf(this->savedSensorConfigsList, oldConfig, 0);
  if (index != -1) {
    this->savedSensorConfigsList.at(static_cast<std::size_t>(index)) = sConfig;
  }

  // update active sensor config
  if (isActive) {
    this->activeSensorConfig = sConfig;
  }

  emit this->sensorConfigurationReplaced(oldConfig, sConfig);
}

/*!
 * \brief SensorConfigurationManager::loadFromConfigFolder
 * Loads all available sensor configs from the config folder at application
 * directory
 */
void SensorConfigurationManager::loadFromConfigFolder() {
  // get application path
  QString appPath = qApp->applicationDirPath();
  QDir appFolder(appPath);

  // check if application folder exists and contains a subdirectory named config
  if (!appFolder.exists() || !appFolder.exists("config/sensorConfigs")) {
    return;
  }

  // create folder object for sensor config folder
  QDir sConfigFolder(appPath.append("/config/sensorConfigs"));
  if (!sConfigFolder.exists()) {
    return;
  }

  // get a list of all json files inside the sensorConfigs folder
  QStringList jsonFilter;
  jsonFilter.append("*.json");
  QStringList fileNames = sConfigFolder.entryList(jsonFilter, QDir::Files);

  // create a list that is filled with the names of the loaded sensor configs
  QStringList sConfigNames;

  // load all files and create a SensorConfiguration object for each of them
  for (const auto& fileName : fileNames) {
    // unreadable or invalid files are skipped
    const QString filePath = sConfigFolder.absoluteFilePath(fileName);
    const auto loaded = configfile::readSensorConfiguration(filePath);
    if (!loaded) {
      continue;
    }
    SensorConfiguration userConfig = *loaded;
    userConfig.setIsSaved(true);

    // check if a sensor config with the same name has been loaded before
    if (sConfigNames.contains(userConfig.getName())) {
      // delete the config file permanently
      QFile::remove(filePath);
      continue;
    }
    sConfigNames.append(toQString(userConfig.getName()));
    this->addSavedSensorConfig(userConfig);

    // add the loaded sensor config to the list of saved configs
    if (!this->savedSensorConfigsMap.contains(userConfig.getName())) {
      this->savedSensorConfigsMap.insert_or_assign(userConfig.getName(),
                                                   userConfig);
      this->savedSensorConfigsList.push_back(userConfig);
    }
  }

  // user tier (~/.openindy/sensorConfigs/) - always writable without elevated
  // permissions, where the GUI "save config" flow actually writes to. A
  // user-tier config sharing a system-tier one's name shadows it in the
  // list, same as PluginRepository - no file is ever touched here, unlike
  // the same-directory dedup above.
  QDir userSConfigFolder(QDir::homePath() + "/.openindy/sensorConfigs");
  if (userSConfigFolder.exists()) {
    foreach (const QString& fileName,
             userSConfigFolder.entryList(jsonFilter, QDir::Files)) {
      const QString filePath = userSConfigFolder.absoluteFilePath(fileName);
      const auto loaded = configfile::readSensorConfiguration(filePath);
      if (!loaded) {
        continue;
      }
      SensorConfiguration userConfig = *loaded;
      userConfig.setIsSaved(true);

      if (this->savedSensorConfigsMap.contains(userConfig.getName())) {
        qWarning() << "SensorConfigurationManager: user sensor config"
                   << filePath << "shadows a system one sharing the name"
                   << userConfig.getName();
        oi::removeOne(
            this->savedSensorConfigsList,
            oi::value(this->savedSensorConfigsMap, userConfig.getName()));
      }
      this->addSavedSensorConfig(userConfig);
      this->savedSensorConfigsMap.insert_or_assign(userConfig.getName(),
                                                   userConfig);
      this->savedSensorConfigsList.push_back(userConfig);
    }
  }

  // get default sensor config
  QString sConfigName = Settings::get("defaultMeasurementDevice");
  if (this->savedSensorConfigsMap.contains(toStd(sConfigName))) {
    this->activeSensorConfig =
        oi::value(this->savedSensorConfigsMap, toStd(sConfigName));
    emit this->activeSensorConfigurationChanged();
  }

  // emit signals
  emit this->sensorConfigurationsChanged();
}

/*!
 * \brief SensorConfigurationManager::synchronize
 * \param other
 */
void SensorConfigurationManager::synchronize(
    const SensorConfigurationManager& other) {
  // do not trigger signals during synchronization
  this->blockSignals(true);

  // remove sensor configs
  this->removeAllSavedSensorConfigs();
  this->removeAllProjectSensorConfigs();

  // add new configs
  std::vector<SensorConfiguration> userConfigs = other.getSavedSensorConfigs();
  std::vector<SensorConfiguration> projectConfigs =
      other.getProjectSensorConfigs();
  foreach (const SensorConfiguration& sConfig, userConfigs) {
    this->addSavedSensorConfig(sConfig);
  }
  foreach (const SensorConfiguration& sConfig, projectConfigs) {
    this->addProjectSensorConfig(sConfig);
  }

  // trigger edits again
  this->blockSignals(false);

  emit this->sensorConfigurationsChanged();
}

/*!
 * \brief SensorConfigurationManager::saveSensorConfig
 * \param sConfig
 * \param save
 */
void SensorConfigurationManager::saveSensorConfig(
    const SensorConfiguration& sConfig, bool save) {
  // ###################################
  // create config file at config folder
  // ###################################

  if (save) {
    // get config folder (create if does not exist yet)
    QString appPath = qApp->applicationDirPath();
    QDir sConfigFolder(appPath.append("/config/sensorConfigs"));
    if (!sConfigFolder.exists()) {
      sConfigFolder.mkpath(".");
    }

    // save sConfig to json file
    configfile::write(
        sConfigFolder.absoluteFilePath(toQString(sConfig.getName() + ".json")),
        oi::codec::encodeSensorConfiguration(sConfig));
  }

  // ########################################
  // add sConfig to the list of saved configs
  // ########################################

  if (!this->savedSensorConfigsMap.contains(sConfig.getName())) {
    this->savedSensorConfigsMap.insert_or_assign(sConfig.getName(), sConfig);
    this->savedSensorConfigsList.push_back(sConfig);
  }

  // ############
  // emit signals
  // ############

  emit this->sensorConfigurationsChanged();
}

/*!
 * \brief SensorConfigurationManager::deleteSensorConfig
 * \param name
 * \param save
 */
void SensorConfigurationManager::deleteSensorConfig(const QString& name,
                                                    bool save) {
  // #####################################
  // remove config file from config folder
  // #####################################

  // get config folder
  if (save) {
    QString appPath = qApp->applicationDirPath();
    QDir sConfigFolder(appPath.append("/config/sensorConfigs"));
    if (sConfigFolder.exists()) {
      // get config file
      QFile configFile(sConfigFolder.absoluteFilePath(name + ".json"));
      if (configFile.exists()) {
        configFile.remove();
      }
    }
  }

  // #############################################
  // remove sConfig from the list of saved configs
  // #############################################

  SensorConfiguration sConfig =
      oi::value(this->savedSensorConfigsMap, toStd(name));
  this->savedSensorConfigsMap.erase(toStd(name));
  oi::removeOne(this->savedSensorConfigsList, sConfig);

  // reset active sensor config
  if (name.compare(toQString(this->activeSensorConfig.getName())) == 0 &&
      this->activeSensorConfig.getIsSaved()) {
    this->activeSensorConfig = SensorConfiguration();
    emit this->activeSensorConfigurationChanged();
  }

  // ############
  // emit signals
  // ############

  emit this->sensorConfigurationsChanged();
}

/*!
 * \brief SensorConfigurationManager::equals
 * \param sConfigA
 * \param sConfigB
 * \return
 */
bool SensorConfigurationManager::equals(const SensorConfiguration& sConfigA,
                                        const SensorConfiguration& sConfigB) {
  // compare general attributes
  if (sConfigA.getName().compare(sConfigB.getName()) != 0 ||
      sConfigA.getIsSaved() != sConfigB.getIsSaved() ||
      sConfigA.getPluginName().compare(sConfigB.getPluginName()) != 0 ||
      sConfigA.getSensorName().compare(sConfigB.getSensorName()) != 0 ||
      sConfigA.getTypeOfSensor() != sConfigB.getTypeOfSensor()) {
    return false;
  }

  // compare accuracy values
  if (!almostEqual(sConfigA.getAccuracy().sigmaAzimuth,
                   sConfigB.getAccuracy().sigmaAzimuth,
                   kAccuracyCompareDecimals) ||
      !almostEqual(sConfigA.getAccuracy().sigmaZenith,
                   sConfigB.getAccuracy().sigmaZenith,
                   kAccuracyCompareDecimals) ||
      !almostEqual(sConfigA.getAccuracy().sigmaDistance,
                   sConfigB.getAccuracy().sigmaDistance,
                   kAccuracyCompareDecimals) ||
      !almostEqual(sConfigA.getAccuracy().sigmaXyz.getAt(0),
                   sConfigB.getAccuracy().sigmaXyz.getAt(0),
                   kAccuracyCompareDecimals) ||
      !almostEqual(sConfigA.getAccuracy().sigmaXyz.getAt(1),
                   sConfigB.getAccuracy().sigmaXyz.getAt(1),
                   kAccuracyCompareDecimals) ||
      !almostEqual(sConfigA.getAccuracy().sigmaXyz.getAt(2),
                   sConfigB.getAccuracy().sigmaXyz.getAt(2),
                   kAccuracyCompareDecimals) ||
      !almostEqual(sConfigA.getAccuracy().sigmaTemp,
                   sConfigB.getAccuracy().sigmaTemp,
                   kAccuracyCompareDecimals) ||
      !almostEqual(sConfigA.getAccuracy().sigmaI, sConfigB.getAccuracy().sigmaI,
                   kAccuracyCompareDecimals) ||
      !almostEqual(sConfigA.getAccuracy().sigmaJ, sConfigB.getAccuracy().sigmaJ,
                   kAccuracyCompareDecimals) ||
      !almostEqual(sConfigA.getAccuracy().sigmaK, sConfigB.getAccuracy().sigmaK,
                   kAccuracyCompareDecimals) ||
      oi::keys(sConfigA.getAccuracy().sigmaUndefined) !=
          oi::keys(sConfigB.getAccuracy().sigmaUndefined)) {
    return false;
  }

  // compare undefined accuracies
  std::vector<std::string> keys =
      oi::keys(sConfigA.getAccuracy().sigmaUndefined);
  foreach (const std::string& key, keys) {
    if (!almostEqual(oi::value(sConfigA.getAccuracy().sigmaUndefined, key),
                     oi::value(sConfigB.getAccuracy().sigmaUndefined, key),
                     kAccuracyCompareDecimals)) {
      return false;
    }
  }

  // compare string parameters
  if (sConfigA.getStringParameter() != sConfigB.getStringParameter()) {
    return false;
  }

  // compare double parameters
  if (oi::keys(sConfigA.getDoubleParameter()) !=
      oi::keys(sConfigB.getDoubleParameter())) {
    return false;
  } else {
    std::vector<std::string> keys = oi::keys(sConfigA.getDoubleParameter());
    foreach (const std::string& key, keys) {
      if (!almostEqual(oi::value(sConfigA.getDoubleParameter(), key),
                       oi::value(sConfigB.getDoubleParameter(), key),
                       kAccuracyCompareDecimals)) {
        return false;
      }
    }
  }

  // compare integer parameters
  if (sConfigA.getIntegerParameter() != sConfigB.getIntegerParameter()) {
    return false;
  }

  // compare available string parameters
  if (sConfigA.getAvailableStringParameter() !=
      sConfigB.getAvailableStringParameter()) {
    return false;
  }

  return true;
}

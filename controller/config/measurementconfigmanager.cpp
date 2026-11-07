#include "measurementconfigmanager.h"

#include <QDebug>
#include <QDir>
#include <QFile>

#include "configfile.h"

/*!
 * \brief MeasurementConfigManager::MeasurementConfigManager
 */
MeasurementConfigManager::MeasurementConfigManager(QObject* parent)
    : QObject(parent) {
  // standard configs are driver-published, not hardcoded here - each
  // scanned sensor's init() declares its own catalog (PluginRepository
  // tags every entry with that sensor's type), so a config only appears
  // at all if some loaded driver actually claims to support it. This is
  // what let "level" default to being offered for every sensor before,
  // including ones (LeicaTachymeter) that can't produce a level reading.
  foreach (MeasurementConfig c, PluginRepository::getAllMeasurementConfigs()) {
    this->configs.insert(c.getKey(), c);
    this->addProjectConfig(c);
  }
}

/*!
 * \brief MeasurementConfigManager::MeasurementConfigManager
 * \param copy
 * \param parent
 */
MeasurementConfigManager::MeasurementConfigManager(
    const MeasurementConfigManager& copy, QObject* parent)
    : configs(copy.configs),
      activeMeasurementConfigs(copy.activeMeasurementConfigs) {}

/*!
 * \brief MeasurementConfigManager::operator =
 * \param copy
 * \return
 */
MeasurementConfigManager& MeasurementConfigManager::operator=(
    const MeasurementConfigManager& copy) {
  this->configs = copy.configs;
  this->activeMeasurementConfigs = copy.activeMeasurementConfigs;
  return *this;
}

/*!
 * \brief MeasurementConfigManager::getCurrentJob
 * \return
 */
const oi::Guarded<OiJob>& MeasurementConfigManager::getCurrentJob() const {
  return this->currentJob;
}

/*!
 * \brief MeasurementConfigManager::setCurrentJob
 * \param job
 */
void MeasurementConfigManager::setCurrentJob(const oi::Guarded<OiJob>& job) {
  if (!job.isNull()) {
    // disconnect current job
    if (!this->currentJob.isNull()) {
      this->disconnectJob();
    }

    this->currentJob = job;
    this->connectJob();
  }
}

MeasurementConfig MeasurementConfigManager::getUserConfig(
    const QString& name) const {
  return configs.value(MeasurementConfigKey(name, ConfigTypes::eUserConfig),
                       MeasurementConfig());
}

MeasurementConfig MeasurementConfigManager::getProjectConfig(
    const QString& name) const {
  return configs.value(MeasurementConfigKey(name, ConfigTypes::eProjectConfig),
                       MeasurementConfig());
}

MeasurementConfig MeasurementConfigManager::getStandardConfig(
    const QString& name) const {
  return configs.value(MeasurementConfigKey(name, ConfigTypes::eStandardConfig),
                       MeasurementConfig());
}

/**
 * @brief MeasurementConfigManager::findConfig find MeasurementConfig by name
 * with fallback
 * @param name
 * @return found MeasurementConfig or invalid MeasurementConfig
 */
MeasurementConfig MeasurementConfigManager::findConfig(
    const QString& name) const {
  MeasurementConfig mc = this->getStandardConfig(name);
  mc = mc.isValid() ? mc : this->getProjectConfig(name);  // fallback
  mc = mc.isValid() ? mc : this->getUserConfig(name);     // fallback
  return mc;
}

MeasurementConfig MeasurementConfigManager::getConfig(
    const MeasurementConfigKey& key) const {
  return configs.value(key, MeasurementConfig());
}

/*!
 * \brief MeasurementConfigManager::getUserConfigs
 * \return
 */
const QList<MeasurementConfig> MeasurementConfigManager::getUserConfigs()
    const {
  QList<MeasurementConfig> l;
  for (const MeasurementConfig c : configs) {
    if (c.isUserConfig()) {
      l.append(c);
    }
  }
  return l;
}

const QList<MeasurementConfig> MeasurementConfigManager::getConfigs() const {
  return configs.values();
}

/*!
 * \brief MeasurementConfigManager::getProjectConfigs
 * \return
 */
const QList<MeasurementConfig> MeasurementConfigManager::getProjectConfigs()
    const {
  QList<MeasurementConfig> l;
  for (const MeasurementConfig c : configs) {
    if (c.isProjectConfig()) {
      l.append(c);
    }
  }
  return l;
}
const QList<QString> MeasurementConfigManager::getUserConfigNames() const {
  QList<QString> l;
  foreach (const MeasurementConfig c, configs) {
    if (c.isUserConfig()) {
      l.append(c.getName());
    }
  }
  return l;
}

/*!
 * \brief MeasurementConfigManager::getProjectConfigs
 * \return
 */
const QList<QString> MeasurementConfigManager::getProjectConfigNames() const {
  QList<QString> l;
  foreach (const MeasurementConfig c, configs) {
    if (c.isProjectConfig()) {
      l.append(c.getName());
    }
  }
  return l;
}
/*!
 * \brief MeasurementConfigManager::getActiveConfig
 * \param type
 * \return
 */
MeasurementConfig MeasurementConfigManager::getActiveConfig(
    const GeometryTypes& type) const {
  return this->activeMeasurementConfigs.value(type, MeasurementConfig());
}

/*!
 * \brief MeasurementConfigManager::saveUserConfig
 * \param mConfig
 */
void MeasurementConfigManager::saveUserConfig(
    const MeasurementConfig& mConfig) {
  // check if mConfig is valid
  if (!mConfig.isValid()) {
    emit this->sendMessage(
        "Cannot add a user measurement configuration with an empty name",
        MessageTypes::eErrorMessage);
    return;
  }

  if (!mConfig.isUserConfig()) {
    emit this->sendMessage(QString("Is not a user measurement configuration %1")
                               .arg(mConfig.getName()),
                           MessageTypes::eErrorMessage);
    return;
  }

  // check if mConfig already exists
  if (this->configs.contains(mConfig.getKey())) {
    emit this->sendMessage(
        QString(
            "A user measurement configuration with the name %1 already exists")
            .arg(mConfig.getName()),
        MessageTypes::eErrorMessage);
    return;
  }

  // save mConfig
  MeasurementConfig userConfig = mConfig;
  userConfig.makeUserConfig();
  this->saveConfig(userConfig);
}

/*!
 * \brief MeasurementConfigManager::addProjectConfig
 * \param mConfig
 */
void MeasurementConfigManager::addProjectConfig(
    const MeasurementConfig& mConfig) {
  // check if mConfig is valid
  if (!mConfig.isValid()) {
    emit this->sendMessage(
        "Cannot add a project measurement configuration with an empty name",
        MessageTypes::eErrorMessage);
    return;
  }

  if (!mConfig.isProjectConfig()) {
    emit this->sendMessage(
        QString("Is not a project measurement configuration %1")
            .arg(mConfig.getName()),
        MessageTypes::eErrorMessage);
    return;
  }

  // check if mConfig already exists
  if (this->configs.contains(mConfig.getKey())) {
    emit this->sendMessage(QString("A project measurement configuration with "
                                   "the name %1 already exists")
                               .arg(mConfig.getName()),
                           MessageTypes::eErrorMessage);
    return;
  }

  MeasurementConfigKey key(mConfig.getName(), ConfigTypes::eStandardConfig);
  if (this->configs.contains(key)) {
    emit this->sendMessage(QString("A standard measurement configuration with "
                                   "the name %1 already exists")
                               .arg(mConfig.getName()),
                           MessageTypes::eErrorMessage);
    return;
  }

  // save mConfig
  MeasurementConfig projectConfig = mConfig;
  this->configs.insert(projectConfig.getKey(), projectConfig);

  emit this->measurementConfigurationsChanged();
}

/*!
 * \brief MeasurementConfigManager::removeMeasurementConfig
 * \param name
 */
void MeasurementConfigManager::removeUserConfig(const QString& name) {
  // check name
  if (name.compare("") == 0) {
    emit this->sendMessage(
        "Cannot remove a measurement configuration with an empty name",
        MessageTypes::eErrorMessage);
    return;
  }

  // check if the measurement config exists
  if (!getUserConfigNames().contains(name)) {
    emit this->sendMessage(
        QString("A measurement configuration with the name %1 does not exist")
            .arg(name),
        MessageTypes::eErrorMessage);
    return;
  }

  // delete mConfig
  this->deleteMeasurementConfig(name);
}

/*!
 * \brief MeasurementConfigManager::removeProjectConfig
 * \param name
 */
void MeasurementConfigManager::removeProjectConfig(const QString& name) {
  MeasurementConfigKey key(name, ConfigTypes::eProjectConfig);
  if (this->configs.contains(key)) {
    this->configs.remove(key);

    emit this->measurementConfigurationsChanged();
  }
}

/*!
 * \brief MeasurementConfigManager::replaceMeasurementConfig
 * \param name
 * \param mConfig
 */
void MeasurementConfigManager::replaceMeasurementConfig(
    const MeasurementConfigKey& oldKey, const MeasurementConfig& mConfig) {
  MeasurementConfig oldConfig = this->getConfig(oldKey);
  if (!oldConfig.isValid()) {
    return;
  }

  // write user config as json file
  if (mConfig.isUserConfig()) {
    this->saveToConfigFolder(oldConfig.getName(), mConfig);
  }

  // replace mConfig in map
  this->configs.remove(oldKey);
  this->configs.insert(mConfig.getKey(), mConfig);
}

void MeasurementConfigManager::saveToConfigFolder(
    const QString& name, const MeasurementConfig& mConfig) {
  // get config folder (create if does not exist yet)
  QString appPath = qApp->applicationDirPath();
  QDir mConfigFolder(appPath.append("/config/measurementConfigs"));
  if (!mConfigFolder.exists()) {
    mConfigFolder.mkpath(".");
  }

  // remove old config file
  QFile oldConfigFile(mConfigFolder.absoluteFilePath(name + ".json"));
  if (oldConfigFile.exists()) {
    oldConfigFile.remove();
  }

  // save mConfig to json file
  configfile::write(mConfigFolder.absoluteFilePath(mConfig.getName() + ".json"),
                    oi::codec::encodeMeasurementConfig(mConfig));
}

/*!
 * \brief MeasurementConfigManager::loadFromConfigFolder
 */
void MeasurementConfigManager::loadFromConfigFolder() {
  // get application path
  QString appPath = qApp->applicationDirPath();
  QDir appFolder(appPath);

  // check if application folder exists and contains a subdirectory named config
  if (!appFolder.exists() || !appFolder.exists("config/measurementConfigs")) {
    return;
  }

  // create folder object for measurement config folder
  QDir mConfigFolder(appPath.append("/config/measurementConfigs"));
  if (!mConfigFolder.exists()) {
    return;
  }

  // get a list of all json files inside the measurementConfigs folder
  QStringList jsonFilter;
  jsonFilter.append("*.json");
  QStringList fileNames = mConfigFolder.entryList(jsonFilter, QDir::Files);

  // create a list that is filled with the names of the loaded measurement
  // configs
  QStringList mConfigNames;

  // load all files and create a MeasurementConfig object for each of them
  for (const auto& fileName : fileNames) {
    // try to parse the file to a MeasurementConfig object; unreadable or
    // invalid files are skipped
    const QString filePath = mConfigFolder.absoluteFilePath(fileName);
    const auto loaded = configfile::readMeasurementConfig(filePath);
    if (!loaded) {
      continue;
    }
    MeasurementConfig userConfig = *loaded;
    if (userConfig.isStandardConfig()) {  // skip standard config from file
      continue;
    }
    userConfig.makeUserConfig();

    // check if a measurement config with the same name has been loaded before
    if (mConfigNames.contains(userConfig.getName())) {
      // delete the config file permanently
      QFile::remove(filePath);
      continue;
    }
    mConfigNames.append(userConfig.getName());

    this->saveUserConfig(userConfig);

    // add the loaded measurement config to the list of saved configs
    if (!this->configs.contains(userConfig.getKey())) {
      this->configs.insert(userConfig.getKey(), userConfig);
    }
  }

  // user tier (~/.openindy/measurementConfigs/) - always writable without
  // elevated permissions. A user-tier config sharing a system-tier one's
  // name shadows it - unlike the system-tier loop above, this never writes
  // any file (saveUserConfig()/saveConfig() hard-code the system directory,
  // calling them here would wrongly duplicate the config into that tier).
  QDir userMConfigFolder(QDir::homePath() + "/.openindy/measurementConfigs");
  if (userMConfigFolder.exists()) {
    QStringList userMConfigNames;
    foreach (const QString& fileName,
             userMConfigFolder.entryList(jsonFilter, QDir::Files)) {
      const QString filePath = userMConfigFolder.absoluteFilePath(fileName);
      const auto loaded = configfile::readMeasurementConfig(filePath);
      if (!loaded || loaded->isStandardConfig()) {
        continue;
      }
      MeasurementConfig userConfig = *loaded;
      userConfig.makeUserConfig();

      if (userMConfigNames.contains(userConfig.getName())) {
        continue;
      }
      userMConfigNames.append(userConfig.getName());

      // shadow a same-named system-tier entry, if any
      foreach (const MeasurementConfigKey& key, this->configs.keys()) {
        if (this->configs.value(key).getName().compare(userConfig.getName()) ==
            0) {
          qWarning() << "MeasurementConfigManager: user measurement config"
                     << filePath << "shadows a system one sharing the name"
                     << userConfig.getName();
          this->configs.remove(key);
          break;
        }
      }
      this->configs.insert(userConfig.getKey(), userConfig);
    }
  }

  // emit signals
  emit this->measurementConfigurationsChanged();
}

/*!
 * \brief MeasurementConfigManager::synchronize
 * \param other
 */
void MeasurementConfigManager::synchronize(
    const MeasurementConfigManager& other) {
  // do not trigger signals during synchronization
  this->blockSignals(true);

  // remove measurement configs
  this->configs.clear();

  // add new configs
  foreach (const MeasurementConfig& mConfig, other.getConfigs()) {
    if (mConfig.isUserConfig()) {
      this->saveUserConfig(mConfig);
    } else {
      this->addProjectConfig(mConfig);
    }
  }

  // trigger edits again
  this->blockSignals(false);

  emit this->measurementConfigurationsChanged();
}

/*!
 * \brief MeasurementConfigManager::saveConfig
 * \param mConfig
 */
void MeasurementConfigManager::saveConfig(const MeasurementConfig& mConfig) {
  // ###################################
  // create config file at config folder
  // ###################################

  // get config folder (create if does not exist yet)
  QString appPath = qApp->applicationDirPath();
  QDir mConfigFolder(appPath.append("/config/measurementConfigs"));
  if (!mConfigFolder.exists()) {
    mConfigFolder.mkpath(".");
  }

  // save mConfig to json file
  configfile::write(mConfigFolder.absoluteFilePath(mConfig.getName() + ".json"),
                    oi::codec::encodeMeasurementConfig(mConfig));

  // ########################################
  // add mConfig to the list of saved configs
  // ########################################

  if (!this->configs.contains(mConfig.getKey())) {  // TODO pull up
    this->configs.insert(mConfig.getKey(), mConfig);
  }

  // ############
  // emit signals
  // ############

  emit this->measurementConfigurationsChanged();  // TODO OI-948 pull up
}

/*!
 * \brief MeasurementConfigManager::deleteMeasurementConfig
 * \param name
 */
void MeasurementConfigManager::deleteMeasurementConfig(const QString& name) {
  // #####################################
  // remove config file from config folder
  // #####################################

  // get config folder
  QString appPath = qApp->applicationDirPath();
  QDir mConfigFolder(appPath.append("/config/measurementConfigs"));
  if (mConfigFolder.exists()) {
    // get config file
    QFile configFile(mConfigFolder.absoluteFilePath(name + ".json"));
    if (configFile.exists()) {
      configFile.remove();
    }
  }

  // #############################################
  // remove mConfig from the list of saved configs
  // #############################################

  MeasurementConfigKey key(name, ConfigTypes::eUserConfig);
  this->configs.remove(key);

  emit this->measurementConfigurationsChanged();
}

/*!
 * \brief MeasurementConfigManager::connectJob
 */
void MeasurementConfigManager::connectJob() {}

/*!
 * \brief MeasurementConfigManager::disconnectJob
 */
void MeasurementConfigManager::disconnectJob() {}

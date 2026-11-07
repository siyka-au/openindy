#include "controller.h"

#include <map>
#include <set>
#include <vector>

#include "featureutil.h"
#include "qtjobadapter.h"
#include "qtstationadapter.h"
/*!
 * \brief Controller::Controller
 * \param parent
 */
Controller::Controller(QObject* parent) : QObject(parent) {}

void Controller::init() {
  // register meta types
  this->registerMetaTypes();

  // load and restore project unit settings
  if (!ProjectConfig::loadProjectPathConfigFile()) {
    this->log("Could not load project path", MessageTypes::eErrorMessage,
              MessageDestinations::eMessageBoxMessage);
  }

  // load config from file
  if (!ProjectConfig::loadProjectSettingsConfigFile()) {
    this->log("Could not load project settings", MessageTypes::eErrorMessage,
              MessageDestinations::eMessageBoxMessage);
  }

  // initialize and connect model manager
  ModelManager::init();
  if (!ModelManager::myInstance.isNull()) {
    QObject::connect(ModelManager::myInstance.data(),
                     &ModelManager::sendMessage, this, &Controller::log,
                     Qt::QueuedConnection);

    // "compress" updateModel signals
    QTimer* timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()),
            &ModelManager::myInstance->getFeatureTableModel(),
            SLOT(updateModelIfRequested()));
    timer->start(250);
  }

  QObject::connect(PluginLoader::getInstance().data(),
                   &PluginLoader::sendMessage, this, &Controller::log,
                   Qt::QueuedConnection);

  // initialize display configs
  this->initDisplayConfigs();

  // initialize config manager
  this->initConfigManager();

#ifdef OI_WEBSOCKETSERVER_ENABLED
  // init and start OpenIndy server
  this->initServer();
  this->startServer();
#endif

  // initialize tool plugins
  this->initToolPlugins();

  this->initDefaults();  // after read measurement confing & after load
                         // functions

  // connect helper objects
  this->connectDataExchanger();
  this->connectFeatureUpdater();
#ifdef OI_WEBSOCKETSERVER_ENABLED
  this->connectRequestHandler();
#endif
}

/*!
 * \brief Controller::~Controller
 */
Controller::~Controller() {
  // stop web socket server thread
#ifdef OI_WEBSOCKETSERVER_ENABLED
  if (this->serverThread.isRunning()) {
    this->serverThread.quit();
    this->serverThread.wait();
  }
#endif

  QPointer<Console> console = Console::getInstance();
  if (!console.isNull()) {
    delete Console::getInstance().data();
  }

  // the stations' sensor workers must not post to the executor member any
  // more once it is destroyed
  if (!this->job.isNull()) {
    this->job->setExecutor(nullptr);
  }
}

/*!
 * \brief Controller::getAvailableTools
 * \return
 */
const QList<QPointer<Tool> >& Controller::getAvailableTools() const {
  return this->toolPlugins;
}

/*!
 * \brief Controller::addFeatures
 * \param attributes
 */
void Controller::addFeatures(const FeatureAttributes& attributes) {
  // check job
  if (this->job.isNull()) {
    this->log("No active job", MessageTypes::eErrorMessage,
              MessageDestinations::eMessageBoxMessage);
    return;
  }

  std::vector<oi::Guarded<FeatureWrapper> > features =
      this->job->addFeatures(attributes);
  if (features.size() == 0) {
    this->log("No feature were created - See console output for details",
              MessageTypes::eErrorMessage,
              MessageDestinations::eMessageBoxMessage);
    emit this->featureCreated(false);
    return;
  }
  emit this->featureCreated(true);

  // check used states of trafo params
  for (const auto& feature : features) {
    if (!feature->getTrafoParam().isNull()) {
      // check if other trafos with this coordinate systems already exist and
      // are active/ used
      for (const auto& j : this->job->getTransformationParametersList()) {
        if ((feature->getTrafoParam()->getStartSystem() ==
                 j->getStartSystem() &&
             feature->getTrafoParam()->getDestinationSystem() ==
                 j->getDestinationSystem()) ||
            (feature->getTrafoParam()->getStartSystem() ==
                 j->getDestinationSystem() &&
             feature->getTrafoParam()->getDestinationSystem() ==
                 j->getStartSystem())) {
          if (j->getFeatureName().compare(
                  feature->getFeature()->getFeatureName()) != 0) {
            if (j->getIsUsed()) {
              feature->getTrafoParam()->setIsUsed(false);
              emit this->requestMessageBoxTrafoParam();
              break;
            }
          }
        }
      }
    }
  }

  // check if there is at least a function or a measurement config that shall be
  // set
  if (attributes.measurementConfig.isValid() ||
      (!attributes.functionPlugin.first.isEmpty() &&
       !attributes.functionPlugin.second.isEmpty())) {
    // create functions and measurement configs for the created features
    this->addFunctionsAndMConfigs(features, attributes.measurementConfig,
                                  attributes.functionPlugin.second,
                                  attributes.functionPlugin.first);
  }

  foreach (oi::Guarded<FeatureWrapper> feature, features) {
    oi::Guarded<Station> station = feature->getStation();
    if (!station.isNull()) {
      oi::Guarded<Station> activeStation = this->job->getActiveStation();
      station->setEstimatedCoordinateSystem(
          activeStation->getCoordinateSystem());
    }
  }
}

/*!
 * \brief Controller::removeFeatures
 * \param featureIds
 */
void Controller::removeFeatures(const std::set<int>& featureIds) {
  // check job
  if (this->job.isNull()) {
    this->log("No active job", MessageTypes::eErrorMessage,
              MessageDestinations::eMessageBoxMessage);
    return;
  }

  // remove features
  if (!this->job->removeFeatures(featureIds)) {
    this->log("Error while removeing features", MessageTypes::eErrorMessage,
              MessageDestinations::eMessageBoxMessage);
  }
}

/*!
 * \brief Controller::setNominalParameters
 * \param featureId
 * \param parameters
 */
void Controller::setNominalParameters(
    const int& featureId,
    const std::map<GeometryParameters, double>& parameters) {
  // check job
  if (this->job.isNull()) {
    this->log("No active job", MessageTypes::eErrorMessage,
              MessageDestinations::eMessageBoxMessage);
    return;
  }

  // get and check the feature by its id
  oi::Guarded<FeatureWrapper> feature = this->job->getFeatureById(featureId);
  if (feature.isNull() || feature->getGeometry().isNull()) {
    this->log(QString("No geometry with the id %1").arg(featureId),
              MessageTypes::eErrorMessage,
              MessageDestinations::eMessageBoxMessage);
    return;
  }

  // update the feature's parameters
  feature->getGeometry()->setUnknownParameters(parameters);

  this->featureUpdater.recalcFeature(feature->getFeature());
}

/*!
 * \brief Controller::setTrafoParamParameters
 * \param featureId
 * \param parameters
 */
void Controller::setTrafoParamParameters(
    const int& featureId,
    const std::map<TrafoParamParameters, double>& parameters) {
  // check job
  if (this->job.isNull()) {
    this->log("No active job", MessageTypes::eErrorMessage,
              MessageDestinations::eMessageBoxMessage);
    return;
  }

  // get and check the feature by its id
  oi::Guarded<FeatureWrapper> feature = this->job->getFeatureById(featureId);
  if (feature.isNull() || feature->getTrafoParam().isNull()) {
    this->log(QString("No trafo param with the id %1").arg(featureId),
              MessageTypes::eErrorMessage,
              MessageDestinations::eMessageBoxMessage);
    return;
  }

  // update the feature's parameters
  feature->getTrafoParam()->setUnknownParameters(parameters);
  feature->getTrafoParam()->setIsSolved(true);
}

/*!
 * \brief Controller::recalcActiveFeature
 */
void Controller::recalcActiveFeature() {
  // check job
  if (this->job.isNull()) {
    return;
  }

  // get and check active feature
  oi::Guarded<FeatureWrapper> feature = this->job->getActiveFeature();
  if (feature.isNull() || feature->getFeature().isNull()) {
    return;
  }

  // recalculate the active feature
  if (!feature->getTrafoParam().isNull()) {
    this->featureUpdater.recalcTrafoParam(feature->getTrafoParam());
  } else {
    this->featureUpdater.recalcFeature(feature->getFeature());
  }
}

/*!
 * \brief Controller::generateOwnedNormalLine
 * UI verification checkpoint: the "check a box in a relationship" trigger
 * for a solved Plane's owned normal line (Stage 7h, D19). No-arg, operates
 * on the active feature - same convention as recalcActiveFeature()/startAim().
 */
void Controller::generateOwnedNormalLine() {
  this->generateOwnedSubGeometry(FeatureTypes::eLineFeature,
                                 "LineFromPlaneNormal", "NormalLine");
}

/*!
 * \brief Controller::generateOwnedOriginPoint
 * See generateOwnedNormalLine() - same mechanism, PointFromPlaneOrigin.
 */
void Controller::generateOwnedOriginPoint() {
  this->generateOwnedSubGeometry(FeatureTypes::ePointFeature,
                                 "PointFromPlaneOrigin", "OriginPoint");
}

/*!
 * \brief Controller::generateOwnedSubGeometry
 * Creates a bare subType feature with no function attached, then attaches
 * functionName as its owned producer via OiJob::addOwnedSubGeometry - the
 * same three-step shape (bare feature -> instantiate producer -> attach as
 * owned) tst_ownedplaneaccessors.cpp already exercises at the API level,
 * now reachable from the UI. Silently no-ops on a missing/unsuitable active
 * feature or an unresolvable producer function, matching
 * recalcActiveFeature()'s own silent-return convention for "nothing to do
 * here" - OiJob::addFeatures()/addOwnedSubGeometry() already emit their own
 * sendMessage() on any error worth surfacing to the user (invalid name, no
 * valid producer, etc).
 * \param subType
 * \param functionName
 * \param namePrefix
 */
void Controller::generateOwnedSubGeometry(const FeatureTypes& subType,
                                          const QString& functionName,
                                          const QString& namePrefix) {
  // check job
  if (this->job.isNull()) {
    return;
  }

  // get and check active feature - must be a solved Plane (the only owner
  // type this checkpoint's producers support; other geometry types are
  // Stage 7h's Haiku-replication follow-up, not this one)
  oi::Guarded<FeatureWrapper> owner = this->job->getActiveFeature();
  if (owner.isNull() || owner->getPlane().isNull() ||
      !owner->getGeometry()->getIsSolved()) {
    return;
  }

  // resolve the producer function's plugin, then instantiate it
  std::vector<sys::Function> candidates =
      PluginRepository::getFunctionsApplicableTo(subType);
  oi::Guarded<Function> producerFunction(nullptr);
  for (const sys::Function& candidate : candidates) {
    if (candidate.name().compare(functionName) == 0) {
      producerFunction = PluginLoader::loadFunctionPlugin(
          candidate.plugin().filePath(), candidate.name());
      break;
    }
  }
  if (producerFunction.isNull()) {
    return;
  }

  // create the bare sub-geometry - no function attached yet, that's what
  // addOwnedSubGeometry below does. Suffix by owner id, not name, since
  // the owner's name is user-editable and not guaranteed unique-safe to
  // derive a second unique name from on a repeat click.
  FeatureAttributes subAttributes;
  subAttributes.typeOfFeature = subType;
  subAttributes.name =
      QString("%1_%2").arg(namePrefix).arg(owner->getFeature()->getId());
  subAttributes.count = 1;
  std::vector<oi::Guarded<FeatureWrapper> > created =
      this->job->addFeatures(subAttributes);
  if (created.size() != 1 || created.at(0).isNull()) {
    return;
  }

  // attach it as owner's owned sub-geometry - mints the producer's id,
  // binds owner as input, records ownership, triggers the initial compute
  this->job->addOwnedSubGeometry(owner, created.at(0), producerFunction);
}

/*!
 * \brief Controller::sensorConfigurationChanged
 * Set a new sensor configuration for the active sensor
 * \param name
 * \param connectSensor
 */
void Controller::setSensorConfig(const SensorConfiguration& sConfig,
                                 bool connectSensor) {
  // check job
  if (this->job.isNull()) {
    return;
  }

  // get and check active station
  oi::Guarded<Station> activeStation = this->getActiveStation();
  if (activeStation.isNull()) {
    this->log("No active station", MessageTypes::eErrorMessage,
              MessageDestinations::eMessageBoxMessage);
    return;
  }

  // get and check the specified sensor config
  if (!sConfig.getIsValid()) {
    this->log("Invalid sensor configuration selected",
              MessageTypes::eErrorMessage,
              MessageDestinations::eMessageBoxMessage);
    return;
  }

  // get and check plugin information
  std::optional<sys::Plugin> plugin =
      PluginRepository::getByName(sConfig.getPluginName());
  if (!plugin.has_value()) {
    this->log(QString("No plugin available with the name %1")
                  .arg(sConfig.getPluginName()),
              MessageTypes::eErrorMessage,
              MessageDestinations::eMessageBoxMessage);
    return;
  }

  // create sensor instance and assign it to the active station
  oi::Guarded<Sensor> sensor = PluginLoader::loadSensorPlugin(
      plugin->filePath(), sConfig.getSensorName());
  if (sensor.isNull()) {
    this->log(QString("No sensor available with the name %1")
                  .arg(sConfig.getSensorName()),
              MessageTypes::eErrorMessage,
              MessageDestinations::eMessageBoxMessage);
    return;
  }
  QObject::connect(QtStationAdapter::of(activeStation),
                   &QtStationAdapter::sensorStatus, this,
                   &Controller::sensorStatus, Qt::UniqueConnection);

  // set active station's sensor
  sensor->setSensorConfiguration(sConfig);
  activeStation->setSensor(sensor);

  // connect the sensor
  if (connectSensor) {
    this->startConnect();
  }

  // update the active sensor config of the config manager
  if (!this->sensorConfigManager.isNull()) {
    this->sensorConfigManager->setActiveSensorConfig(sConfig);
  }
}

/*!
 * \brief Controller::removeActiveStationSensor
 * Resets the sensor of the active station
 */
void Controller::removeActiveStationSensor() {
  // check job
  if (this->job.isNull()) {
    return;
  }

  // get and check active station
  oi::Guarded<Station> station = this->getActiveStation();
  if (station.isNull()) {
    return;
  }

  // reset the sensor of the active station
  station->resetSensor();
}

/*!
 * \brief Controller::sensorConfigurationsEdited
 * Synchronize the sensor config manager with the given one
 * \param manager
 */
void Controller::sensorConfigurationsEdited(
    const SensorConfigurationManager& manager) {
  // check job
  if (this->job.isNull()) {
    return;
  }

  // check sensor config manager
  if (this->sensorConfigManager.isNull()) {
    return;
  }

  // synchronize the managers
  this->sensorConfigManager->synchronize(manager);
}

/*!
 * \brief Controller::sensorConfigurationUpdated
 * Update the current sensor configuration of the active station
 * \param sConfig
 */
void Controller::sensorConfigurationUpdated(
    const SensorConfiguration& sConfig) {
  // check job
  if (this->job.isNull()) {
    return;
  }

  // get and check active station
  oi::Guarded<Station> activeStation = this->getActiveStation();
  if (activeStation.isNull()) {
    this->log("No active station", MessageTypes::eErrorMessage,
              MessageDestinations::eMessageBoxMessage);
    return;
  }

  // set sensor configuration
  activeStation->setSensorConfiguration(sConfig);
}

/*!
 * \brief Controller::measurementConfigurationChanged
 * \param mConfig
 */
void Controller::measurementConfigurationChanged(
    const MeasurementConfig& mConfig) {
  // check job
  if (this->job.isNull()) {
    return;
  }

  // get and check active feature
  oi::Guarded<FeatureWrapper> activeFeature = this->job->getActiveFeature();
  if (activeFeature.isNull() || activeFeature->getGeometry().isNull()) {
    this->log("No active geometry selected", MessageTypes::eErrorMessage,
              MessageDestinations::eMessageBoxMessage);
    return;
  }

  // get and check the specified measurement config
  if (!mConfig.isValid()) {
    this->log("No measurement configuration selected",
              MessageTypes::eErrorMessage,
              MessageDestinations::eMessageBoxMessage);
    return;
  }

  // set measurement config for the active feature
  activeFeature->getGeometry()->setMeasurementConfig(mConfig.getKey());
}
void Controller::saveUserConfig(const MeasurementConfig& mConfig) {
  if (mConfig.isUserConfig() && !mConfig.isStandardConfig()) {
    measurementConfigManager->saveUserConfig(
        mConfig);  // "add" to config/measurementConfigs folder
  } else {
    this->log(QString("Cannot save a measurement configuration \"%1\" because "
                      "is it not vaild user config")
                  .arg(mConfig.getName()),
              MessageTypes::eErrorMessage,
              MessageDestinations::eMessageBoxMessage);
  }
}
/*!
 * \brief Controller::setActiveStation
 * \param featureId
 */
/*void Controller::setActiveStation(const int &featureId){

}*/

/*!
 * \brief Controller::setActiveCoordinateSystem
 * \param featureId
 */
/*void Controller::setActiveCoordinateSystem(const int &featureId){

}*/

/*!
 * \brief Controller::importNominals
 * \param params
 */
void Controller::importNominals(const ExchangeParams& params) {
  // try to start import nominal task
  if (!this->exchanger.importData(params)) {
    return;
  }

  // show loading dialog during import
  emit this->nominalImportStarted();
}

/*!
 * \brief Controller::exportFeatures
 * \param params
 */
void Controller::exportFeatures(const ExchangeParams& params) {
  // try to start export features
  if (!this->exchanger.exportData(params)) {
    return;
  }
}

/*!
 * \brief Controller::importObservations
 * \param filename
 */
void Controller::importObservations(const QString& filename) {
  // try to start import observations task
  if (!this->exchanger.importObservations(filename)) {
    return;
  }

  // show loading dialog during import
  emit this->observationImportStarted();
}

/*!
 * \brief Controller::removeObservations
 * \param featureId
 */
void Controller::removeObservations(const int& featureId) {
  // check job
  if (this->job.isNull()) {
    this->log("No job available", MessageTypes::eErrorMessage,
              MessageDestinations::eMessageBoxMessage);
    return;
  }

  this->job->removeObservations(featureId);
}

void Controller::enableObservations(const int& featureId) {
  // check job
  if (this->job.isNull()) {
    this->log("No job available", MessageTypes::eErrorMessage,
              MessageDestinations::eMessageBoxMessage);
    return;
  }

  this->job->enableObservations(featureId);
}
void Controller::disableObservations(const int& featureId) {
  // check job
  if (this->job.isNull()) {
    this->log("No job available", MessageTypes::eErrorMessage,
              MessageDestinations::eMessageBoxMessage);
    return;
  }

  this->job->disableObservations(featureId);
}

void Controller::removeObservationsById(const std::vector<int> selectedIds) {
  // check job
  if (this->job.isNull()) {
    this->log("No job available", MessageTypes::eErrorMessage,
              MessageDestinations::eMessageBoxMessage);
    return;
  }

  this->job->removeObservations(
      this->job->getActiveFeature()->getFeature()->getId(), selectedIds);
}

/*!
 * \brief Controller::removeAllObservations
 */
void Controller::removeAllObservations() {
  // check job
  if (this->job.isNull()) {
    this->log("No job available", MessageTypes::eErrorMessage,
              MessageDestinations::eMessageBoxMessage);
    return;
  }

  this->job->removeAllObservations();
}

/*!
 * \brief Controller::setFeatureTableColumnConfig
 * \param config
 */
void Controller::setFeatureTableColumnConfig(
    const FeatureTableColumnConfig& config) {
  ModelManager::setFeatureTableColumnConfig(config);
}

/*!
 * \brief Controller::setTrafoParamColumnConfig
 * \param config
 */
void Controller::setTrafoParamColumnConfig(
    const TrafoParamTableColumnConfig& config) {
  ModelManager::setTrafoParamColumnConfig(config);
}

/*!
 * \brief Controller::setObservationTableColumnConfig
 * \param config
 */
void Controller::setObservationTableColumnConfig(
    const ObservationTableColumnConfig& config) {
  ModelManager::setObservationTableColumnConfig(config);
}

/*!
 * \brief Controller::setReadingTableColumnConfig
 * \param config
 */
void Controller::setReadingTableColumnConfig(
    const ReadingTableColumnConfig& config) {
  ModelManager::setReadingTableColumnConfig(config);
}

/*!
 * \brief Controller::setParameterDisplayConfig
 * \param config
 */
void Controller::setParameterDisplayConfig(
    const ParameterDisplayConfig& config) {
  ModelManager::setParameterDisplayConfig(config);
  emit this->updateStatusBar();
}

/*!
 * \brief Controller::addBundleSystem
 */
void Controller::addBundleSystem() {  // TODO OI-932 remove method / OI-1009
                                      // maybe

  // check job
  if (this->job.isNull()) {
    this->log("No job available", MessageTypes::eErrorMessage,
              MessageDestinations::eMessageBoxMessage);
    return;
  }

  // create feature name
  const QString bundleName = "Bundle01";

  // create feature attributes
  FeatureAttributes bundleAttr;
  bundleAttr.count = 1;
  bundleAttr.typeOfFeature = FeatureTypes::eCoordinateSystemFeature;
  bundleAttr.name = bundleName;
  bundleAttr.isBundleSystem = true;

  // add feature
  this->job->addFeatures(bundleAttr);

  FeatureUtil util;
  if (!util.createInitialBundleTransformation(job)) {
    this->log("initial bundle not created", MessageTypes::eWarningMessage,
              MessageDestinations::eConsoleMessage);
  }
}

/*!
 * \brief Controller::removeBundleSystem
 * \param id
 */
void Controller::removeBundleSystem(const int& id) {
  // check job
  if (this->job.isNull()) {
    this->log("No job available", MessageTypes::eErrorMessage,
              MessageDestinations::eMessageBoxMessage);
    return;
  }

  // remove bundle system
  this->job->removeFeature(id);
}

/*!
 * \brief Controller::getBundleTemplate
 * \return
 */
QJsonObject Controller::getBundleTemplate() {
  // check job
  if (this->job.isNull()) {
    this->log("No job available", MessageTypes::eErrorMessage,
              MessageDestinations::eMessageBoxMessage);
    return QJsonObject();
  }
  return this->job->getActiveBundleTemplate();
}

/*!
 * \brief Controller::getBundleAdjustment
 * Returns the bundle plugin of the coordinate system defined by bundleId
 * \return
 */
oi::Guarded<oi::BundleAdjustment> Controller::getBundleAdjustment() {
  // check job
  if (this->job.isNull()) {
    this->log("No job available", MessageTypes::eErrorMessage,
              MessageDestinations::eMessageBoxMessage);
    return oi::Guarded<oi::BundleAdjustment>(nullptr);
  }
  return this->job->getActiveBundleAdjustment();
}

/*!
 * \brief Controller::updateBundleAdjustment
 * Updates the bundle plugin of the coordinate system defined by bundleId
 * \param param
 */
void Controller::updateBundleAdjustment(
    const QJsonObject& param) {  // TODO bundle scalar parameters remove later

  // check job
  if (this->job.isNull()) {
    this->log("No job available", MessageTypes::eErrorMessage,
              MessageDestinations::eMessageBoxMessage);
    return;
  }

  // get and check bundle plugin
  oi::Guarded<BundleAdjustment> bundlePlugin = this->getBundleAdjustment();
  if (bundlePlugin.isNull()) {
    return;
  }

  QJsonArray params;
  /* TODO bundle scalar parameters remove later
      //update scalar parameters
      ScalarInputParams scalarParams;
      QJsonArray params = param.value("integerParameter").toArray();
      for(int i = 0; i < params.size(); i++){
          QJsonObject intParam = params.at(i).toObject();
          if(intParam.contains("name") && intParam.contains("value")){
              scalarParams.intParameter.insert(intParam.value("name").toString(),
     intParam.value("value").toInt());
          }
      }
      params = param.value("doubleParameter").toArray();
      for(int i = 0; i < params.size(); i++){
          QJsonObject doubleParam = params.at(i).toObject();
          if(doubleParam.contains("name") && doubleParam.contains("value")){
              scalarParams.doubleParameter.insert(doubleParam.value("name").toString(),
     doubleParam.value("value").toDouble());
          }
      }
      params = param.value("stringParameter").toArray();
      for(int i = 0; i < params.size(); i++){
          QJsonObject stringParam = params.at(i).toObject();
          if(stringParam.contains("name") && stringParam.contains("value")){
              scalarParams.stringParameter.insert(stringParam.value("name").toString(),
     stringParam.value("value").toString());
          }
      }
  */
  // update input stations
  std::vector<BundleStation> inputStations;
  params = param.value("inputStations").toArray();
  for (auto&& param : params) {
    QJsonObject station = param.toObject();
    if (!station.contains("id") || !station.contains("used") ||
        !station.value("used").toBool()) {
      continue;
    }
    BundleStation bundleStation;
    bundleStation.id = station.value("id").toInt();
    bundleStation.tx = station.value("tx").toBool();
    bundleStation.ty = station.value("ty").toBool();
    bundleStation.tz = station.value("tz").toBool();
    bundleStation.rx = station.value("rx").toBool();
    bundleStation.ry = station.value("ry").toBool();
    bundleStation.rz = station.value("rz").toBool();
    bundleStation.m = station.value("m").toBool();
    inputStations.push_back(bundleStation);
  }

  // set up input parameters
  // bundlePlugin->setScalarInputParams(scalarParams);
  bundlePlugin->setInputStations(inputStations);
}

/*!
 * \brief Controller::loadBundleTemplate
 * \param bundleTemplate
 */
void Controller::loadBundleTemplate(const QJsonObject& bundleTemplate) {
  // check job
  if (this->job.isNull()) {
    this->log("No job available", MessageTypes::eErrorMessage,
              MessageDestinations::eMessageBoxMessage);
    return;
  }

  // get bundle system
  oi::Guarded<CoordinateSystem> bundleSystem =
      this->job->getActiveBundleSystem();
  if (bundleSystem.isNull()) {
    this->log(QString("No bundle system available"),
              MessageTypes::eErrorMessage,
              MessageDestinations::eMessageBoxMessage);
    return;
  }

  // get bundle and plugin name
  QString bundleName, pluginName;
  bundleName =
      bundleTemplate.value("plugin").toObject().value("name").toString();
  pluginName =
      bundleTemplate.value("plugin").toObject().value("pluginName").toString();

  // get and check plugin information
  std::optional<sys::Plugin> plugin = PluginRepository::getByName(pluginName);
  if (!plugin.has_value()) {
    this->log(QString("No plugin available with the name %1").arg(pluginName),
              MessageTypes::eErrorMessage,
              MessageDestinations::eMessageBoxMessage);
    return;
  }

  // load bundle plugin
  oi::Guarded<BundleAdjustment> bundlePlugin =
      PluginLoader::loadBundleAdjustmentPlugin(plugin->filePath(), bundleName);
  if (bundlePlugin.isNull()) {
    this->log(QString("Cannot load bundle template %1")
                  .arg(bundleTemplate.value("name").toString()),
              MessageTypes::eErrorMessage,
              MessageDestinations::eMessageBoxMessage);
    return;
  }

  // set up bundle plugin
  bundleSystem->setBundleTemplate(bundleTemplate);
  bundleSystem->setBundlePlugin(bundlePlugin);

  this->log(QString("Bundle template %1 loaded successfully")
                .arg(bundleTemplate.value("name").toString()),
            MessageTypes::eInformationMessage,
            MessageDestinations::eConsoleMessage);
}

/*!
 * \brief Controller::runBundle
 */
void Controller::runBundle() {
  // check job
  if (this->job.isNull()) {
    this->log("No job available", MessageTypes::eErrorMessage,
              MessageDestinations::eMessageBoxMessage);
    return;
  }

  oi::Guarded<oi::BundleAdjustment> bundleAdjustment =
      this->getBundleAdjustment();
  if (bundleAdjustment.isNull()) {
    this->log("No bundle adjustment available", MessageTypes::eErrorMessage,
              MessageDestinations::eMessageBoxMessage);
    return;
  }

  oi::Guarded<CoordinateSystem> bundleSystem =
      bundleAdjustment->getBundleSystem();

  // check bundle plugin
  if (bundleSystem->getBundlePlugin().isNull()) {
    this->log(QString("No bundle plugin loaded for bundle %1")
                  .arg(bundleSystem->getFeatureName()),
              MessageTypes::eErrorMessage,
              MessageDestinations::eMessageBoxMessage);
    return;
  }

  // calculate bundle adjustment
  try {
    if (!this->featureUpdater.recalcBundle(bundleSystem)) {
      this->featureUpdater.switchCoordinateSystem();

      this->log(QString("Error when calculating bundle %1")
                    .arg(bundleSystem->getFeatureName()),
                MessageTypes::eErrorMessage,
                MessageDestinations::eMessageBoxMessage);
    } else {
      this->log(QString("Bundle %1 successfully calculated")
                    .arg(bundleSystem->getFeatureName()),
                MessageTypes::eInformationMessage,
                MessageDestinations::eMessageBoxMessage);
    }
  } catch (BundleException& e) {
    this->featureUpdater.switchCoordinateSystem();

    oi::Guarded<FeatureWrapper> feature =
        this->job->getFeatureById(e.getFeatureId());
    QString featureName =
        feature.isNull() ? "" : feature->getFeature()->getFeatureName() + ": ";
    this->log(featureName.append(e.what()), MessageTypes::eErrorMessage,
              MessageDestinations::eMessageBoxMessage);
  } catch (std::exception& e) {
    this->featureUpdater.switchCoordinateSystem();

    this->log(e.what(), MessageTypes::eErrorMessage,
              MessageDestinations::eMessageBoxMessage);
  } catch (...) {
    this->featureUpdater.switchCoordinateSystem();

    this->log("unexpected: cannot recalc bundle", MessageTypes::eErrorMessage,
              MessageDestinations::eMessageBoxMessage);
  }

  // update station status
  oi::Guarded<Station> activeStation = job->getActiveStation();
  if (activeStation.isNull()) {
    return;
  }

  StationStatusData data;
  data.id = activeStation->getId();
  data.name = activeStation->getFeatureName();
  emit stationStatus(data);
}

/*!
 * \brief check if digest has changed and update digest in job
 */
bool Controller::hasProjectDigestChanged() {
  if (this->job.isNull()) {
    return false;
  }
  QString preDigest = this->job->getDigest();
  projectExchanger.saveProject(this->job);
  return preDigest !=
         job->getDigest();  // not equal, that means project data changed
}

/*!
 * \brief Controller::saveProject
 */
void Controller::saveProject() {
  // check job
  if (this->job.isNull()) {
    this->log("No job available", MessageTypes::eErrorMessage,
              MessageDestinations::eMessageBoxMessage);
    return;
  }

  QList<QPointer<Tool> > tools = this->getAvailableTools();
  QList<QPointer<Tool> >::iterator toolIt;
  for (toolIt = tools.begin(); toolIt != tools.end(); ++toolIt) {
    const QPointer<Tool>& tool = *toolIt;
    if (!tool->saveProjectEnabled()) {
      this->log(QString("Saving project denied by tool: %1")
                    .arg(tool->getMetaData().name),
                MessageTypes::eErrorMessage,
                MessageDestinations::eMessageBoxMessage);
      return;
    }
  }

  // get and check name and file path
  QString name = this->job->getJobName();
  QPointer<QFileDevice> fileDevice = this->job->getJobDevice();
  if (name.compare("") == 0 || fileDevice.isNull()) {
    emit this->saveAsTriggered();
    return;
  }

  QMutexLocker locker(&saveProjectMutex);  // synchronize write file

  // serialise the project
  const QByteArray project = projectExchanger.saveProject(this->job);

  if (project.isEmpty()) {
    this->log("Error while creating the project file",
              MessageTypes::eErrorMessage,
              MessageDestinations::eMessageBoxMessage);
    return;
  }

  // write the project file
  QSaveFile saveFile(fileDevice->fileName());
  if (saveFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    if (saveFile.write(project) != project.size() || !saveFile.commit()) {
      this->log(QString("Cannot write file \"%1\"").arg(fileDevice->fileName()),
                MessageTypes::eErrorMessage,
                MessageDestinations::eMessageBoxMessage);
      return;
    }

    this->log(QString("OpenIndy project \"%1\" successfully stored.")
                  .arg(fileDevice->fileName()),
              MessageTypes::eInformationMessage,
              MessageDestinations::eStatusBarMessage);
  } else {
    this->log(QString("Cannot open file  \"%1\"").arg(fileDevice->fileName()),
              MessageTypes::eInformationMessage,
              MessageDestinations::eStatusBarMessage);
    QMessageBox msgBox;
    msgBox.setText("data source could not be found.");
    msgBox.setInformativeText(
        "Please check your saved data, before you continue.");
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setDefaultButton(QMessageBox::Ok);
    int ret = msgBox.exec();
    return;
  }
}

/*!
 * \brief Controller::saveProject
 * \param fileName
 */
void Controller::saveProject(const QString& fileName) {
  // check job
  if (this->job.isNull()) {
    this->log("No job available", MessageTypes::eErrorMessage,
              MessageDestinations::eMessageBoxMessage);
    return;
  }

  // set name
  QFileInfo info(fileName);
  if (!info.absoluteDir().exists()) {
    this->log("Directory not exists", MessageTypes::eErrorMessage,
              MessageDestinations::eMessageBoxMessage);
    return;
  }
  this->job->setJobName(fileName);

  // set device
  this->job->setJobDevice(new QFile(fileName));

  this->saveProject();

  emit this->currentJobChanged();  // maybe changed TODO: emit if
                                   // this->saveProject(); is successful
}

/*!
 * \brief Controller::loadProject
 * \param projectName
 * \param device
 */
void Controller::loadProject(const QString& projectName,
                             const QPointer<QFileDevice>& device) {
  // check device
  if (device.isNull()) {
    this->log("No device", MessageTypes::eErrorMessage,
              MessageDestinations::eMessageBoxMessage);
    return;
  }

  // read the project file
  QByteArray project;
  if (device->open(QIODevice::ReadOnly)) {
    project = device->readAll();
    device->close();
  } else {
    this->log("Error while opening OpenIndy project file.",
              MessageTypes::eErrorMessage,
              MessageDestinations::eMessageBoxMessage);
    return;
  }

  // load the project
  const ProjectExchanger::LoadResult loaded =
      projectExchanger.loadProject(project);
  oi::Guarded<OiJob> newJob = loaded.job;

  // check job
  if (newJob.isNull()) {
    this->log(QString("Error while loading OpenIndy project file: %1")
                  .arg(loaded.error),
              MessageTypes::eErrorMessage,
              MessageDestinations::eMessageBoxMessage);
    return;
  }

  bool compatible = false;
  QString msg;
  switch (newJob->checkCompatibilty()) {
    case CompatibilyCheckResult::eCheckResult_match:
    case CompatibilyCheckResult::eCheckResult_oi_gt_job:
      compatible = true;
      break;
    case CompatibilyCheckResult::eCheckResult_job_wo_valid_version:
      compatible = false;
      msg =
          "This OpenIndy project has no vaild version. Continue opening "
          "project?";
      break;
    case CompatibilyCheckResult::eCheckResult_oi_wo_valid_version:
      compatible = false;
      msg = "This OpenIndy has no vaild version. Continue opening project?";
      break;
    case CompatibilyCheckResult::eCheckResult_oi_lt_job:
      compatible = false;
      msg =
          "This OpenIndy version is older than OpenIndy project version. "
          "Please use newer OpenIndy version. Continue opening project?";
      break;
    case CompatibilyCheckResult::eCheckResult_job_lt_oi_22_1:
      compatible = false;
      msg =
          "This OpenIndy version is newer than OpenIndy project version. "
          "Please migrate project otherwise project measurement config will "
          "not work propperly. "
          "Continue opening project?";
      break;
    default:
      compatible = false;
      msg =
          "This OpenIndy version is not compatible with OpenIndy project "
          "version. Continue opening project?";
      break;
  }

  if (compatible ||
      (emit this->showMessageBox(msg, MessageTypes::eQuestionMessage) ==
       QMessageBox::Yes)) {
    // set project meta data
    newJob->setJobDevice(device);
    newJob->setJobName(projectName);

    // delete old job
    if (!this->job.isNull()) {
      delete this->job;
    }

    // set new job
    this->setJob(newJob);

    // switch to active coordinate system
    this->featureUpdater.switchCoordinateSystem();

    // connect active station
    this->activeStationChangedCallback();

    // load bundle templates
    QList<QJsonObject> templates = this->loadBundleTemplates();
    if (!templates.isEmpty()) {
      QJsonObject bundleTemplate = templates.first();  // only one should exists

      if (this->job->getActiveBundleAdjustment()
              .isNull()) {  // no bundle result was saved
        // load plugin and set plugin to bundle system "Bundle01"
        // see MainWindow::initBundleView()
        std::vector<oi::Guarded<FeatureWrapper> > bundleSystems =
            this->job->getFeaturesByName("Bundle01");
        if (!bundleSystems.empty()) {
          oi::Guarded<CoordinateSystem> bundleSystem =
              bundleSystems.front()->getCoordinateSystem();

          QString bundleName, pluginName;
          bundleName = bundleTemplate.value("plugin")
                           .toObject()
                           .value("name")
                           .toString();
          pluginName = bundleTemplate.value("plugin")
                           .toObject()
                           .value("pluginName")
                           .toString();

          // get and check plugin information
          std::optional<sys::Plugin> plugin =
              PluginRepository::getByName(pluginName);
          if (!plugin.has_value()) {
            this->log(
                QString("No plugin available with the name %1").arg(pluginName),
                MessageTypes::eErrorMessage,
                MessageDestinations::eMessageBoxMessage);
            return;
          }

          // load bundle plugin
          oi::Guarded<BundleAdjustment> bundlePlugin =
              PluginLoader::loadBundleAdjustmentPlugin(plugin->filePath(),
                                                       bundleName);
          if (bundlePlugin.isNull()) {
            this->log(QString("Cannot load bundle template %1")
                          .arg(bundleTemplate.value("name").toString()),
                      MessageTypes::eErrorMessage,
                      MessageDestinations::eMessageBoxMessage);
            return;
          }

          // get bundle system
          if (bundleSystem.isNull()) {
            this->log(QString("No bundle system available"),
                      MessageTypes::eErrorMessage,
                      MessageDestinations::eMessageBoxMessage);
            return;
          }
          // set up bundle plugin
          bundleSystem->setBundleTemplate(bundleTemplate);
          bundleSystem->setBundlePlugin(bundlePlugin);

          // initially add all stations to bundle
          // see void MainWindow::bundleSelectionChanged()
          std::vector<BundleStation> bundleStations;
          foreach (const oi::Guarded<Station>& station,
                   this->job->getStationsList()) {
            // create initial BundleStation
            BundleStation bundleStation;
            bundleStation.id = station->getId();
            bundleStation.tx = false;
            bundleStation.ty = false;
            bundleStation.tz = false;
            bundleStation.rx = false;
            bundleStation.ry = false;
            bundleStation.rz = false;
            bundleStation.m = true;
            bundleStations.push_back(bundleStation);
          }

          bundlePlugin->setInputStations(bundleStations);

        } else {
          // no Bundle01 available, should not appear
        }

      } else {
        // load template
        this->job->getActiveBundleSystem()->setBundleTemplate(bundleTemplate);
      }
    }

    this->log(QString("OpenIndy project \"%1\" successfully loaded.")
                  .arg(device->fileName()),
              MessageTypes::eInformationMessage,
              MessageDestinations::eConsoleMessage);
  } else {
    this->log(QString("OpenIndy project \"%1\" was not loaded.")
                  .arg(device->fileName()),
              MessageTypes::eInformationMessage,
              MessageDestinations::eConsoleMessage);
  }
}

/*!
 * \brief Controller::createDefaultJob
 * \return
 */
const oi::Guarded<OiJob>& Controller::createDefaultJob() {
  // create job with a station and a nominal system
  oi::Guarded<OiJob> job = new OiJob();
  this->setJob(job);

  oi::Guarded<FeatureWrapper> stationFeature = new FeatureWrapper();
  oi::Guarded<Station> station = new Station();
  station->setFeatureName("STATION01");
  station->setGroupName("00_Stations");
  stationFeature->setStation(station);

  oi::Guarded<FeatureWrapper> systemFeature = new FeatureWrapper();
  oi::Guarded<CoordinateSystem> system = new CoordinateSystem();
  system->setFeatureName("PART");
  systemFeature->setCoordinateSystem(system);

  job->addFeature(stationFeature);
  job->addFeature(systemFeature);

  // activate features
  station->setActiveStationState(true);
  station->getCoordinateSystem()->setActiveCoordinateSystemState(true);

  this->addBundleSystem();

  return this->job;
}

/*!
 * \brief Controller::startConnect
 */
void Controller::startConnect() {
  // check current job
  if (this->job.isNull()) {
    return;
  }

  // get and check active station
  oi::Guarded<Station> activeStation = this->getActiveStation();
  if (activeStation.isNull()) {
    this->log("No active station", MessageTypes::eErrorMessage,
              MessageDestinations::eMessageBoxMessage);
    return;
  }

  // inform about start of sensor action
  emit this->sensorActionStarted("connecting sensor...");

  // connect sensor
  activeStation->connectSensor();
}

/*!
 * \brief Controller::startDisconnect
 */
void Controller::startDisconnect() {
  // check current job
  if (this->job.isNull()) {
    return;
  }

  // get and check active station
  oi::Guarded<Station> activeStation = this->getActiveStation();
  if (activeStation.isNull()) {
    this->log("No active station", MessageTypes::eErrorMessage,
              MessageDestinations::eMessageBoxMessage);
    return;
  }

  // inform about start of sensor action
  emit this->sensorActionStarted("disconnecting sensor...");

  // disconnect sensor
  activeStation->disconnectSensor();
}

void Controller::startMeasurement() { this->_startMeasurement(false); }

/*!
 * \brief Controller::startMeasurement
 */
void Controller::_startMeasurement(bool dummyPoint) {
  oi::Guarded<Station> activeStation = getConnectedActiveStation();
  if (activeStation.isNull()) {
    return;
  }

  // get and check active feature
  oi::Guarded<FeatureWrapper> activeFeature = this->job->getActiveFeature();
  if (activeFeature.isNull() || activeFeature->getGeometry().isNull()) {
    this->log("No active feature", MessageTypes::eErrorMessage,
              MessageDestinations::eMessageBoxMessage);
    return;
  }

  //"measure this nominal": swap in a fresh actual before measuring
  //(D17/D19 - a nominal has no fit to measure into, so a read-only
  // actual realising it is created first)
  if (!activeFeature->getGeometry()->getOwnerDescription().has_value()) {
    oi::Guarded<FeatureWrapper> actual =
        this->createActualForNominal(activeFeature->getGeometry());
    if (actual.isNull() || actual->getGeometry().isNull()) {
      this->log("Cannot create actual for nominal", MessageTypes::eErrorMessage,
                MessageDestinations::eMessageBoxMessage);
      return;
    }
    activeFeature = actual;
    activeFeature->getFeature()->setActiveFeatureState(true);
  }

  // inform about start of sensor action
  emit this->sensorActionStarted("performing measurement...",
                                 SensorAction::eSensorActionMeasure, true);

  // perform measurement
  int id = activeFeature->getGeometry()->getId();
  MeasurementConfig mConfig = this->measurementConfigManager->getConfig(
      activeFeature->getGeometry()->getMeasurementConfig());

  // the authoritative check, not just the UI's convenience filter - a
  // geometry's staged config can predate the connected sensor (loaded from
  // a job file, or picked before the sensor was swapped), so this must be
  // re-checked here rather than trusted from whatever the UI last showed.
  // An untyped config (generic, or predating the sensor-type tag) is always
  // allowed, same rule the UI filter uses.
  const SensorType activeSensorType = activeStation->getActiveSensorType();
  if (mConfig.getSensorType().isValid() && activeSensorType.isValid() &&
      !(mConfig.getSensorType() == activeSensorType)) {
    this->log(QString("Measurement config \"%1\" is not compatible with the "
                      "connected sensor")
                  .arg(mConfig.getName()),
              MessageTypes::eErrorMessage,
              MessageDestinations::eMessageBoxMessage);
    return;
  }

  mConfig.setTransientData(
      "isDummyPoint",
      dummyPoint);  // use MeasurementConfig for "transportation"
  activeStation->measure(id, mConfig);
}

/*!
 * \brief Controller::startMove
 * \param reading
 */
void Controller::startMove(const Reading& reading) {
  oi::Guarded<Station> activeStation = getConnectedActiveStation();
  if (activeStation.isNull()) {
    return;
  }

  // get and check active feature
  oi::Guarded<FeatureWrapper> activeFeature = this->job->getActiveFeature();
  if (activeFeature.isNull() || activeFeature->getGeometry().isNull()) {
    this->log("No active feature", MessageTypes::eErrorMessage,
              MessageDestinations::eMessageBoxMessage);
    return;
  }

  // inform about start of sensor action
  emit this->sensorActionStarted("moving sensor...",
                                 SensorAction::eSensorActionMove);

  // move sensor
  if (reading.getTypeOfReading() == ReadingTypes::eCartesianReading) {
    activeStation->move(reading.getCartesianReading().xyz.getAt(0),
                        reading.getCartesianReading().xyz.getAt(1),
                        reading.getCartesianReading().xyz.getAt(2), false);
    return;
  } else if (reading.getTypeOfReading() == ReadingTypes::ePolarReading) {
    activeStation->move(reading.getPolarReading().azimuth,
                        reading.getPolarReading().zenith,
                        reading.getPolarReading().distance, false, false);
    return;
  }

  emit this->sensorActionFinished(false,
                                  "Selected reading type is not supported");
}

/*!
 * \brief Controller::startAim
 */
void Controller::startAim() {
  // check current job
  if (this->job.isNull()) {
    return;
  }

  // get and check active feature
  oi::Guarded<FeatureWrapper> activeFeature = this->job->getActiveFeature();
  if (activeFeature.isNull() || activeFeature->getGeometry().isNull()) {
    this->log("No active feature", MessageTypes::eErrorMessage,
              MessageDestinations::eMessageBoxMessage);
    return;
  }

  // get and check active coordinate system
  oi::Guarded<CoordinateSystem> activeCoordinateSystem =
      this->job->getActiveCoordinateSystem();
  if (activeCoordinateSystem.isNull()) {
    this->log("No active coordinate system", MessageTypes::eErrorMessage,
              MessageDestinations::eMessageBoxMessage);
    return;
  }

  // get and check active station
  oi::Guarded<Station> activeStation = this->getActiveStation();
  if (activeStation.isNull() || activeStation->getCoordinateSystem().isNull()) {
    this->log("No active station", MessageTypes::eErrorMessage,
              MessageDestinations::eMessageBoxMessage);
    return;
  }

  // check sensor
  if (!activeStation->getIsSensorConnected()) {
    this->log("No sensor connected to the active station",
              MessageTypes::eErrorMessage,
              MessageDestinations::eMessageBoxMessage);
    return;
  }

  // transform position of the active feature into the station coordinate system
  if (!activeFeature->getGeometry()->hasPosition()) {
    this->log("Active feature has no position to aim",
              MessageTypes::eErrorMessage,
              MessageDestinations::eMessageBoxMessage);
    return;
  }

  // create trafo controller instance to get the transformation matrix
  TrafoController trafoController;
  OiMat t(4, 4);
  OiVec pos;

  // transform the active feature into the station coordinate system
  // switch between actual and nominal if the active feature is not solved
  oi::Guarded<CoordinateSystem> cs =
      !activeStation->getCoordinateSystem().isNull() &&
              !activeStation->getCoordinateSystem()->getIsSolved() &&
              !activeStation->getEstimatedCoordinateSystem().isNull()
          ? activeStation->getEstimatedCoordinateSystem()
          : activeStation->getCoordinateSystem();

  if (!activeFeature->getGeometry()
           ->getOwnerDescription()
           .has_value()) {  // nominal (user-owned, D19)

    // transform the nominal into the station coordinate system
    if (trafoController.getTransformationMatrix(
            t, activeFeature->getGeometry()->getNominalSystem(), cs)) {
      pos = activeFeature->getGeometry()->getPosition().getVectorH();
      pos = t * pos;
    }

  } else {  // actual (fit-owned, D19)

    // transform the actual into the station coordinate system
    if (activeFeature->getGeometry()->getIsSolved() &&
        trafoController.getTransformationMatrix(t, activeCoordinateSystem,
                                                cs)) {
      pos = activeFeature->getGeometry()->getPosition().getVectorH();
      pos = t * pos;
    } else if (activeFeature->getGeometry()->getRealises().has_value() &&
               !activeFeature->getGeometry()->getRealises()->isNull()) {
      // fall back to the nominal this actual realises
      const oi::Guarded<Geometry> nominal =
          activeFeature->getGeometry()->getRealises().value();
      if (nominal->hasPosition() && trafoController.getTransformationMatrix(
                                        t, nominal->getNominalSystem(), cs)) {
        pos = nominal->getPosition().getVectorH();
        pos = t * pos;
      }
    }
  }

  // aim the active feature if a valid position has been found
  if (pos.getSize() == 4) {
    emit this->sensorActionStarted("moving sensor...",
                                   SensorAction::eSensorActionMove);
    activeStation->move(pos.getAt(0), pos.getAt(1), pos.getAt(2), false);
  }
}

/*!
 * \brief Controller::startAimAndMeasure
 */
void Controller::startAimAndMeasure() {
  // check current job
  if (this->job.isNull()) {
    return;
  }

  // get and check active feature
  oi::Guarded<FeatureWrapper> activeFeature = this->job->getActiveFeature();
  if (activeFeature.isNull() || activeFeature->getGeometry().isNull()) {
    this->log("No active feature", MessageTypes::eErrorMessage,
              MessageDestinations::eMessageBoxMessage);
    return;
  }

  // get and check active coordinate system
  oi::Guarded<CoordinateSystem> activeCoordinateSystem =
      this->job->getActiveCoordinateSystem();
  if (activeCoordinateSystem.isNull()) {
    this->log("No active coordinate system", MessageTypes::eErrorMessage,
              MessageDestinations::eMessageBoxMessage);
    return;
  }

  // get and check active station
  oi::Guarded<Station> activeStation = this->getActiveStation();
  if (activeStation.isNull() || activeStation->getCoordinateSystem().isNull()) {
    this->log("No active station", MessageTypes::eErrorMessage,
              MessageDestinations::eMessageBoxMessage);
    return;
  }

  // check sensor
  if (!activeStation->getIsSensorConnected()) {
    this->log("No sensor connected to the active station",
              MessageTypes::eErrorMessage,
              MessageDestinations::eMessageBoxMessage);
    return;
  }

  // transform position of the active feature into the station coordinate system
  if (!activeFeature->getGeometry()->hasPosition()) {
    this->log("Active feature has no position to aim",
              MessageTypes::eErrorMessage,
              MessageDestinations::eMessageBoxMessage);
    return;
  }

  // create trafo controller instance to get the transformation matrix
  TrafoController trafoController;
  OiMat t(4, 4);
  OiVec pos;

  // transform the active feature into the station coordinate system
  // switch between actual and nominal if the active feature is not solved
  if (!activeFeature->getGeometry()
           ->getOwnerDescription()
           .has_value()) {  // nominal (user-owned, D19)

    // transform the nominal into the station coordinate system
    if (trafoController.getTransformationMatrix(
            t, activeFeature->getGeometry()->getNominalSystem(),
            activeStation->getCoordinateSystem())) {
      pos = activeFeature->getGeometry()->getPosition().getVectorH();
      pos = t * pos;
    }

  } else {  // actual (fit-owned, D19)

    // transform the actual into the station coordinate system
    if (activeFeature->getGeometry()->getIsSolved() &&
        trafoController.getTransformationMatrix(
            t, activeCoordinateSystem, activeStation->getCoordinateSystem())) {
      pos = activeFeature->getGeometry()->getPosition().getVectorH();
      pos = t * pos;
    } else if (activeFeature->getGeometry()->getRealises().has_value() &&
               !activeFeature->getGeometry()->getRealises()->isNull()) {
      // fall back to the nominal this actual realises
      const oi::Guarded<Geometry> nominal =
          activeFeature->getGeometry()->getRealises().value();
      if (nominal->hasPosition() && trafoController.getTransformationMatrix(
                                        t, nominal->getNominalSystem(),
                                        activeStation->getCoordinateSystem())) {
        pos = nominal->getPosition().getVectorH();
        pos = t * pos;
      }
    }
  }

  // aim the active feature if a valid position has been found
  if (pos.getSize() == 4) {
    emit this->sensorActionStarted("moving sensor...");
    activeStation->move(
        pos.getAt(0), pos.getAt(1), pos.getAt(2), true,
        activeFeature->getGeometry()->getId(),
        this->measurementConfigManager->getConfig(
            activeFeature->getGeometry()->getMeasurementConfig()));
  }
}

/*!
 * \brief Controller::startToggleSight
 */
void Controller::startToggleSight() {
  oi::Guarded<Station> activeStation = getConnectedActiveStation();
  if (activeStation.isNull()) {
    return;
  }

  // inform about start of sensor action
  emit this->sensorActionStarted("toggling sight orientation...");

  // toggle sight
  activeStation->toggleSight();
}

/*!
 * \brief Controller::startInitialize
 */
void Controller::startInitialize() {
  oi::Guarded<Station> activeStation = getConnectedActiveStation();
  if (activeStation.isNull()) {
    return;
  }

  // inform about start of sensor action
  emit this->sensorActionStarted("initializing sensor...");

  // initialize sensor
  activeStation->initialize();
}

/*!
 * \brief Controller::startHome
 */
void Controller::startHome() {
  oi::Guarded<Station> activeStation = getConnectedActiveStation();
  if (activeStation.isNull()) {
    return;
  }

  // inform about start of sensor action
  emit this->sensorActionStarted("setting sensor to home position...");

  // set sensor to home position
  activeStation->home();
}

/*!
 * \brief Controller::startCompensation
 */
void Controller::startCompensation() {
  oi::Guarded<Station> activeStation = getConnectedActiveStation();
  if (activeStation.isNull()) {
    return;
  }

  // inform about start of sensor action
  emit this->sensorActionStarted("starting compensation...");

  // start compensation
  activeStation->compensation();
}

/*!
 * \brief Controller::startChangeMotorState
 */
void Controller::startChangeMotorState() {
  oi::Guarded<Station> activeStation = getConnectedActiveStation();
  if (activeStation.isNull()) {
    return;
  }

  // inform about start of sensor action
  emit this->sensorActionStarted("changing motor state...");

  // change motor state
  activeStation->motorState();
}

/*!
 * \brief Controller::startCustomAction
 * \param task
 */
void Controller::startCustomAction(const QString& task) {
  oi::Guarded<Station> activeStation = getConnectedActiveStation();
  if (activeStation.isNull()) {
    return;
  }

  // inform about start of sensor action
  emit this->sensorActionStarted(QString("custom action: %1 ...").arg(task));

  // do self defined action
  activeStation->selfDefinedAction(task);
}

/**
 * @brief Controller::getConnectedActiveStation
 * @return connected active station or 0
 */
oi::Guarded<Station> Controller::getConnectedActiveStation() {
  // check current job
  if (this->job.isNull()) {
    return nullptr;
  }

  // get and check active station
  oi::Guarded<Station> activeStation = this->getActiveStation();
  if (activeStation.isNull()) {
    this->log("No active station", MessageTypes::eErrorMessage,
              MessageDestinations::eMessageBoxMessage);
    return nullptr;
  }

  // check sensor
  if (!activeStation->getIsSensorConnected()) {
    this->log("No sensor connected to the active station",
              MessageTypes::eErrorMessage,
              MessageDestinations::eMessageBoxMessage);
    return nullptr;
  }

  return activeStation;
}

oi::Guarded<Station> Controller::getActiveStation() {
  return this->job->getActiveStation();
}

/*!
 * \brief Controller::startReadingStream
 * \param streamFormat
 */
void Controller::startReadingStream(ReadingTypes streamFormat) {
  oi::Guarded<Station> activeStation = getConnectedActiveStation();
  if (activeStation.isNull()) {
    return;
  }

  // start streaming
  activeStation->setStreamFormat(streamFormat);
  activeStation->startReadingStream();
}

/*!
 * \brief Controller::stopReadingStream
 */
void Controller::stopReadingStream() {
  oi::Guarded<Station> activeStation = getConnectedActiveStation();
  if (activeStation.isNull()) {
    return;
  }

  // stop streaming
  activeStation->stopReadingStream();
}

void Controller::finishMeasurement() {
  qDebug() << "Controller::finishMeasurement()";
  oi::Guarded<Station> activeStation = getConnectedActiveStation();
  if (activeStation.isNull()) {
    return;
  }

  qDebug() << "Controller::finishMeasurement() emit";
  // finish Measurement streaming
  activeStation->finishMeasurement();
}
/*!
 * \brief Controller::log
 * Prints a message to the specified destination
 * \param msg
 * \param msgType
 * \param msgDest
 */
void Controller::log(const QString& msg, const MessageTypes& msgType,
                     const MessageDestinations& msgDest) {
  switch (msgDest) {
    case MessageDestinations::eConsoleMessage:
      Console::getInstance()->addLine(msg, msgType);
      break;
    case MessageDestinations::eMessageBoxMessage:
      Console::getInstance()->addLine(msg, msgType);
      emit this->showMessageBox(msg, msgType);
      break;
    case MessageDestinations::eStatusBarMessage:
      Console::getInstance()->addLine(msg, msgType);
      emit this->showStatusMessage(msg, msgType);
      break;
  }
}

/*!
 * \brief Controller::initConfigs
 */
void Controller::initConfigs() { this->initConfigManager(); }

/*!
 * \brief Controller::activeStationChangedCallback
 */
void Controller::activeStationChangedCallback() {
  // check job
  if (this->job.isNull()) {
    return;
  }

  // get and check active station
  oi::Guarded<Station> activeStation = job->getActiveStation();
  if (activeStation.isNull()) {
    return;
  }

  StationStatusData data;
  data.id = activeStation->getId();
  data.name = activeStation->getFeatureName();
  emit stationStatus(data);

  // disconnect all stations
  std::vector<oi::Guarded<Station> > stations = this->job->getStationsList();
  foreach (const oi::Guarded<Station>& station, stations) {
    // check station
    if (station.isNull()) {
      continue;
    }

    // disconnect all slots from signals
    QtStationAdapter* adapter = QtStationAdapter::of(station);
    QObject::disconnect(adapter, &QtStationAdapter::commandFinished, nullptr,
                        nullptr);
    QObject::disconnect(adapter, &QtStationAdapter::measurementFinished,
                        nullptr, nullptr);
    QObject::disconnect(adapter, &QtStationAdapter::measurementDone, nullptr,
                        nullptr);
    QObject::disconnect(adapter, &QtStationAdapter::sensorMessage, nullptr,
                        nullptr);
  }

  // connect sensor action results of active station
  QtStationAdapter* adapter = QtStationAdapter::of(activeStation);
  QObject::connect(adapter, &QtStationAdapter::commandFinished, this,
                   &Controller::sensorActionFinished, Qt::AutoConnection);
  QObject::connect(adapter, &QtStationAdapter::measurementFinished, this,
                   &Controller::measurementFinished, Qt::AutoConnection);
  QObject::connect(adapter, &QtStationAdapter::measurementDone, this,
                   &Controller::measurementDone, Qt::AutoConnection);
  QObject::connect(adapter, &QtStationAdapter::sensorMessage, this,
                   &Controller::log, Qt::AutoConnection);
}

/*!
 * \brief Controller::measurementFinished
 * \param geomId
 * \param readings
 */
void Controller::measurementFinished(
    const int& geomId, const std::vector<oi::Guarded<Reading> >& readings) {
  // check job
  if (this->job.isNull()) {
    return;
  }

  // get and check feature with the id geomId
  oi::Guarded<FeatureWrapper> feature = this->job->getFeatureById(geomId);
  if (feature.isNull() || feature->getGeometry().isNull()) {
    foreach (const oi::Guarded<Reading>& reading, readings) {
      if (!reading.isNull()) {
        delete reading;
      }
    }
    return;
  }

  // the geometry the observations actually get attached to - normally
  // geomId itself, unless this measurement was triggered by a path that
  // skipped _startMeasurement's nominal->actual swap (aim-and-measure
  // moves straight to a geometry id), in which case an actual is
  // created here instead (D17/D19: a nominal has no fit to measure into)
  int targetId = geomId;

  if (!feature->getGeometry()->getOwnerDescription().has_value()) {
    oi::Guarded<FeatureWrapper> actual =
        this->createActualForNominal(feature->getGeometry());
    feature = actual;

    // check feature
    if (feature.isNull() || feature->getGeometry().isNull() ||
        !feature->getGeometry()->getOwnerDescription().has_value()) {
      foreach (const oi::Guarded<Reading>& reading, readings) {
        if (!reading.isNull()) {
          delete reading;
        }
      }
      return;
    }

    targetId = feature->getGeometry()->getId();
  }

  // D12/Stage 7f+7c-ii: OiJob::addMeasurementResults no longer feeds a
  // fixed position on the target's first function - it always creates a
  // tagged Point and lets tag resolution feed whatever function(s)
  // declare a matching role, of any kind (every fit function but
  // BestFitPoint now declares ePointElement, tag-resolved, per Stage
  // 7c-ii). The old "first function must be a FitFunction" gate is not
  // just unnecessary now, it is actively wrong - it would reject
  // measuring straight into a plane's own BestFitPlane, which is exactly
  // the case this stage exists to make work. addMeasurementResults itself
  // reports (with a message) if the target feature cannot be resolved at
  // all.

  // add observations
  this->job->addMeasurementResults(targetId, readings);

  // complete the measurement task by informing MainWindow about its success
  emit this->measurementCompleted();
}

/*!
 * \brief Controller::setJob
 * \param job
 */
void Controller::setJob(const oi::Guarded<OiJob>& job) {
  // check job
  if (job.isNull()) {
    return;
  }

  // delete old job
  if (!this->job.isNull()) {
    delete this->job;
  }

  // set and connect new job
  this->job = job;

  // the stations' sensor results are delivered on the UI thread
  this->job->setExecutor(&this->executor);

  // log messages to console
  QObject::connect(QtJobAdapter::of(this->job.data()),
                   &QtJobAdapter::sendMessage, this, &Controller::log,
                   Qt::AutoConnection);

  // active feature changes
  QObject::connect(QtJobAdapter::of(this->job.data()),
                   &QtJobAdapter::activeFeatureChanged, this,
                   &Controller::activeFeatureChanged, Qt::AutoConnection);
  QObject::connect(QtJobAdapter::of(this->job.data()),
                   &QtJobAdapter::activeStationChanged, this,
                   &Controller::activeStationChanged, Qt::AutoConnection);
  QObject::connect(
      QtJobAdapter::of(this->job.data()), &QtJobAdapter::activeStationChanged,
      this, &Controller::activeStationChangedCallback, Qt::AutoConnection);
  QObject::connect(QtJobAdapter::of(this->job.data()),
                   &QtJobAdapter::activeCoordinateSystemChanged, this,
                   &Controller::activeCoordinateSystemChanged,
                   Qt::AutoConnection);

  // feature(s) added or removed
  QObject::connect(QtJobAdapter::of(this->job.data()),
                   &QtJobAdapter::featureSetChanged, this,
                   &Controller::featureSetChanged, Qt::AutoConnection);
  QObject::connect(QtJobAdapter::of(this->job.data()),
                   &QtJobAdapter::coordSystemSetChanged, this,
                   &Controller::coordSystemSetChanged, Qt::AutoConnection);
  QObject::connect(QtJobAdapter::of(this->job.data()),
                   &QtJobAdapter::stationSetChanged, this,
                   &Controller::stationSetChanged, Qt::AutoConnection);
  QObject::connect(QtJobAdapter::of(this->job.data()),
                   &QtJobAdapter::trafoParamSetChanged, this,
                   &Controller::trafoParamSetChanged, Qt::AutoConnection);
  QObject::connect(QtJobAdapter::of(this->job.data()),
                   &QtJobAdapter::geometrySetChanged, this,
                   &Controller::geometrySetChanged, Qt::AutoConnection);

  // group(s) added or removed
  QObject::connect(QtJobAdapter::of(this->job.data()),
                   &QtJobAdapter::availableGroupsChanged, this,
                   &Controller::availableGroupsChanged, Qt::AutoConnection);
  QObject::connect(QtJobAdapter::of(this->job.data()),
                   &QtJobAdapter::activeGroupChanged, this,
                   &Controller::activeGroupChanged, Qt::AutoConnection);

  // general feature attributes changed
  QObject::connect(QtJobAdapter::of(this->job.data()),
                   &QtJobAdapter::featureAttributesChanged, this,
                   &Controller::featureAttributesChanged, Qt::AutoConnection);

  // feature specific attributes changed
  QObject::connect(QtJobAdapter::of(this->job.data()),
                   &QtJobAdapter::featureNameChanged, this,
                   &Controller::featureNameChanged, Qt::AutoConnection);
  QObject::connect(QtJobAdapter::of(this->job.data()),
                   &QtJobAdapter::featureGroupChanged, this,
                   &Controller::featureGroupChanged, Qt::AutoConnection);
  QObject::connect(QtJobAdapter::of(this->job.data()),
                   &QtJobAdapter::featureCommentChanged, this,
                   &Controller::featureCommentChanged, Qt::AutoConnection);
  QObject::connect(QtJobAdapter::of(this->job.data()),
                   &QtJobAdapter::featureIsUpdatedChanged, this,
                   &Controller::featureIsUpdatedChanged, Qt::AutoConnection);
  QObject::connect(QtJobAdapter::of(this->job.data()),
                   &QtJobAdapter::featureIsSolvedChanged, this,
                   &Controller::featureIsSolvedChanged, Qt::AutoConnection);
  QObject::connect(QtJobAdapter::of(this->job.data()),
                   &QtJobAdapter::featureFunctionsChanged, this,
                   &Controller::featureFunctionsChanged, Qt::AutoConnection);
  QObject::connect(QtJobAdapter::of(this->job.data()),
                   &QtJobAdapter::featureUsedForChanged, this,
                   &Controller::featureUsedForChanged, Qt::AutoConnection);
  QObject::connect(QtJobAdapter::of(this->job.data()),
                   &QtJobAdapter::featurePreviouslyNeededChanged, this,
                   &Controller::featurePreviouslyNeededChanged,
                   Qt::AutoConnection);

  // geometry specific attributes changed
  QObject::connect(QtJobAdapter::of(this->job.data()),
                   &QtJobAdapter::geometryIsCommonChanged, this,
                   &Controller::geometryIsCommonChanged, Qt::AutoConnection);
  QObject::connect(QtJobAdapter::of(this->job.data()),
                   &QtJobAdapter::geometryRealisesChanged, this,
                   &Controller::geometryRealisesChanged, Qt::AutoConnection);
  QObject::connect(QtJobAdapter::of(this->job.data()),
                   &QtJobAdapter::geometryObservationsChanged, this,
                   &Controller::geometryObservationsChanged,
                   Qt::AutoConnection);
  QObject::connect(QtJobAdapter::of(this->job.data()),
                   &QtJobAdapter::geometryNominalSystemChanged, this,
                   &Controller::geometryNominalSystemChanged,
                   Qt::AutoConnection);
  QObject::connect(QtJobAdapter::of(this->job.data()),
                   &QtJobAdapter::geometryStatisticChanged, this,
                   &Controller::geometryStatisticChanged, Qt::AutoConnection);
  QObject::connect(QtJobAdapter::of(this->job.data()),
                   &QtJobAdapter::geometrySimulationDataChanged, this,
                   &Controller::geometrySimulationDataChanged,
                   Qt::AutoConnection);
  QObject::connect(QtJobAdapter::of(this->job.data()),
                   &QtJobAdapter::geometryMeasurementConfigChanged, this,
                   &Controller::geometryMeasurementConfigChanged,
                   Qt::AutoConnection);

  // coordinate system specific attributes changed
  QObject::connect(QtJobAdapter::of(this->job.data()),
                   &QtJobAdapter::systemObservationsChanged, this,
                   &Controller::systemObservationsChanged, Qt::AutoConnection);
  QObject::connect(QtJobAdapter::of(this->job.data()),
                   &QtJobAdapter::systemTrafoParamsChanged, this,
                   &Controller::systemTrafoParamsChanged, Qt::AutoConnection);
  QObject::connect(QtJobAdapter::of(this->job.data()),
                   &QtJobAdapter::systemsNominalsChanged, this,
                   &Controller::systemsNominalsChanged, Qt::AutoConnection);

  // station specific attributes changed
  QObject::connect(QtJobAdapter::of(this->job.data()),
                   &QtJobAdapter::stationSensorChanged, this,
                   &Controller::stationSensorChanged, Qt::AutoConnection);

  // transformation parameter specific attributes changed
  QObject::connect(QtJobAdapter::of(this->job.data()),
                   &QtJobAdapter::trafoParamParametersChanged, this,
                   &Controller::trafoParamParametersChanged,
                   Qt::AutoConnection);
  QObject::connect(QtJobAdapter::of(this->job.data()),
                   &QtJobAdapter::trafoParamSystemsChanged, this,
                   &Controller::trafoParamSystemsChanged, Qt::AutoConnection);
  QObject::connect(QtJobAdapter::of(this->job.data()),
                   &QtJobAdapter::trafoParamIsUsedChanged, this,
                   &Controller::trafoParamIsUsedChanged, Qt::AutoConnection);
  /*QObject::connect(QtJobAdapter::of(this->job.data()),
     &QtJobAdapter::trafoParamValidTimeChanged, this,
     &Controller::trafoParamValidTimeChanged, Qt::AutoConnection);*/
  /*QObject::connect(QtJobAdapter::of(this->job.data()),
     &QtJobAdapter::trafoParamIsMovementChanged, this,
     &Controller::trafoParamIsMovementChanged, Qt::AutoConnection);*/

  // pass the new job around
  ModelManager::setCurrentJob(this->job);
  this->exchanger.setCurrentJob(this->job);
  this->featureUpdater.setCurrentJob(this->job);
#ifdef OI_WEBSOCKETSERVER_ENABLED
  this->requestHandler.setCurrentJob(this->job);
#endif
  if (!this->measurementConfigManager.isNull()) {
    this->measurementConfigManager->setCurrentJob(this->job);
  }

  emit this->currentJobChanged();
  emit this->setCurrentJob(
      this->job);  // set current job to other modules or tools
}

/*!
 * \brief Controller::getActiveFeature
 * \return
 */
oi::Guarded<FeatureWrapper> Controller::getActiveFeature() {
  return this->job->getActiveFeature();
}

/*!
 * \brief Controller::initDisplayConfigs
 * Sets up initial display configs and passes them to model manager
 */
void Controller::initDisplayConfigs() {
  // create default configs
  FeatureTableColumnConfig featureTableColumnConfig(
      ProjectConfig::getDisplayColumns());
  TrafoParamTableColumnConfig trafoParamTableColumnConfig;
  ObservationTableColumnConfig observationTableColumnConfig;
  ReadingTableColumnConfig readingTableColumnConfig;
  ParameterDisplayConfig parameterDisplayConfig =
      ProjectConfig::getParameterDisplayConfig();

  // pass the default configs to model manager
  ModelManager::setFeatureTableColumnConfig(featureTableColumnConfig);
  ModelManager::setTrafoParamColumnConfig(trafoParamTableColumnConfig);
  ModelManager::setObservationTableColumnConfig(observationTableColumnConfig);
  ModelManager::setReadingTableColumnConfig(readingTableColumnConfig);
  ModelManager::setParameterDisplayConfig(parameterDisplayConfig);
}

/*!
 * \brief Controller::initConfigManager
 */
void Controller::initConfigManager() {
  if (this->sensorConfigManager.isNull()) {
    // load configs from config folder
    this->sensorConfigManager = new SensorConfigurationManager();
    this->sensorConfigManager->loadFromConfigFolder();
    this->measurementConfigManager = new MeasurementConfigManager();
    this->measurementConfigManager->loadFromConfigFolder();

    // pass config managers to model manager
    ModelManager::setSensorConfigManager(this->sensorConfigManager);
    ModelManager::setMeasurementConfigManager(this->measurementConfigManager);

    // pass config manager to project exchanger
    projectExchanger.setMeasurementConfigManager(
        this->measurementConfigManager);

    // pass config manager to request handler
#ifdef OI_WEBSOCKETSERVER_ENABLED
    this->requestHandler.setMeasurementConfigManager(
        this->measurementConfigManager);
    this->requestHandler.setSensorConfigManager(this->sensorConfigManager);
#endif
    // connect config manager
    QObject::connect(this->sensorConfigManager.data(),
                     &SensorConfigurationManager::sendMessage, this,
                     &Controller::log, Qt::AutoConnection);
    QObject::connect(this->measurementConfigManager.data(),
                     &MeasurementConfigManager::sendMessage, this,
                     &Controller::log, Qt::AutoConnection);

    this->exchanger.setMesaurementConfigManager(this->measurementConfigManager);
  }
}

/*!
 * \brief Controller::initToolPlugins
 */
void Controller::initToolPlugins() {
  // get available tools
  std::vector<sys::Tool> toolPlugins = PluginRepository::getAllTools();

  // load all tool plugins and add them to the list of tool plugins
  this->toolPlugins.clear();
  foreach (const sys::Tool& tool, toolPlugins) {
    QPointer<Tool> toolPlugin =
        PluginLoader::loadToolPlugin(tool.plugin().filePath(), tool.name());
    if (!toolPlugin.isNull()) {
      toolPlugin->setAttribute(Qt::WA_DeleteOnClose, false);
      this->connectToolPlugin(toolPlugin);
      this->toolPlugins.append(toolPlugin);
    }
  }
}

void Controller::initDefaults() { Settings::init(); }

/*!
 * \brief Controller::registerMetaTypes
 * Registers meta types to be able to use them in signal slot connections
 */
void Controller::registerMetaTypes() {
  qRegisterMetaType<oi::MessageTypes>("MessageTypes");
  qRegisterMetaType<oi::MessageDestinations>("MessageDestinations");
  qRegisterMetaType<oi::MeasurementConfig>("MeasurementConfig");
  qRegisterMetaType<oi::SensorConfiguration>("SensorConfiguration");
  qRegisterMetaType<std::vector<oi::Guarded<oi::Reading> > >(
      "std::vector<oi::Guarded<Reading> >");
  qRegisterMetaType<oi::Guarded<oi::Feature> >("oi::Guarded<Feature>");
  qRegisterMetaType<oi::Guarded<oi::Function> >("oi::Guarded<Function>");
  qRegisterMetaType<oi::Guarded<oi::FeatureWrapper> >(
      "oi::Guarded<FeatureWrapper>");
  qRegisterMetaType<oi::Guarded<oi::Observation> >("oi::Guarded<Observation>");
  qRegisterMetaType<oi::OiRequestResponse>("OiRequestResponse");
  qRegisterMetaType<ParameterDisplayConfig>("ParameterDisplayConfig");

  qRegisterMetaType<oi::Sensor>("Sensor");
  qRegisterMetaType<oi::Guarded<oi::Sensor> >("oi::Guarded<Sensor>");
  qRegisterMetaType<oi::ReadingTypes>("ReadingTypes");
  qRegisterMetaType<std::vector<oi::ReadingTypes> >(
      "std::vector<ReadingTypes>");
  qRegisterMetaType<oi::SensorType>("SensorType");
  qRegisterMetaType<std::vector<oi::SensorType> >("std::vector<SensorType>");
  qRegisterMetaType<oi::ConnectionKind>("ConnectionKind");
  qRegisterMetaType<std::vector<oi::ConnectionKind> >(
      "std::vector<ConnectionKind>");
  qRegisterMetaType<oi::SensorFunctions>("SensorFunctions");
  qRegisterMetaType<std::vector<oi::SensorFunctions> >(
      "std::vector<SensorFunctions>");
  qRegisterMetaType<oi::AcquisitionMode>("AcquisitionMode");
  qRegisterMetaType<std::vector<oi::AcquisitionMode> >(
      "std::vector<AcquisitionMode>");
  qRegisterMetaType<oi::LocalPoint>("LocalPoint");
  qRegisterMetaType<oi::CalibratedConstellation>("CalibratedConstellation");
  // SensorAction crosses the SensorWorker thread boundary in commandFinished();
  // this previously registered the sensor *type* under this name, so the enum
  // actually being queued was never registered at all
  qRegisterMetaType<oi::SensorAction>("SensorAction");
}

#ifdef OI_WEBSOCKETSERVER_ENABLED
/*!
 * \brief Controller::initServer
 */
void Controller::initServer() {
  // create server object
  this->webSocketServer = new OiWebSocketServer();

  // connect server object
  QObject::connect(this, &Controller::startWebSocketServer,
                   this->webSocketServer, &OiWebSocketServer::startServer,
                   Qt::QueuedConnection);
  QObject::connect(this, &Controller::stopWebSocketServer,
                   this->webSocketServer, &OiWebSocketServer::stopServer,
                   Qt::QueuedConnection);
  QObject::connect(this->webSocketServer, &OiWebSocketServer::sendMessage, this,
                   &Controller::log, Qt::QueuedConnection);
  QObject::connect(this->webSocketServer, &OiWebSocketServer::sendRequest,
                   &this->requestHandler, &OiRequestHandler::receiveRequest,
                   Qt::QueuedConnection);
  QObject::connect(&this->requestHandler, &OiRequestHandler::sendResponse,
                   this->webSocketServer, &OiWebSocketServer::receiveResponse,
                   Qt::QueuedConnection);

  // move server object to thread
  if (!this->serverThread.isRunning()) {
    this->serverThread.start();
  }
  this->webSocketServer->moveToThread(&this->serverThread);
}

/*!
 * \brief Controller::startServer
 */
void Controller::startServer() {
  // check server instance
  if (this->webSocketServer.isNull()) {
    return;
  }

  emit this->startWebSocketServer();
}

/*!
 * \brief Controller::stopServer
 */
void Controller::stopServer() {
  // check server instance
  if (this->webSocketServer.isNull()) {
    return;
  }

  emit this->stopWebSocketServer();
}
#endif

/*!
 * \brief Controller::createActualForNominal
 * "Measure this nominal" (domain-model.md §4, D17): always creates a new
 * actual - many actuals may realise the same nominal over repeated
 * inspection, so there is no "the" existing actual to reuse or update.
 * \param nominal
 * \return the new actual, or null on failure
 */
oi::Guarded<FeatureWrapper> Controller::createActualForNominal(
    const oi::Guarded<Geometry>& nominal) {
  // check job and nominal
  if (this->job.isNull() || nominal.isNull() ||
      nominal->getFeatureWrapper().isNull()) {
    return oi::Guarded<FeatureWrapper>(nullptr);
  }

  const FeatureTypes type = nominal->getFeatureWrapper()->getFeatureTypeEnum();

  // find a free name - actuals no longer share the nominal's name (that
  // name-matching was the mechanism that let one object be ticked as
  // both nominal and actual in the old model)
  QString baseName = QString("%1 actual").arg(nominal->getFeatureName());
  QString name = baseName;
  int suffix = 2;
  while (!this->job->validateFeatureName(name, type)) {
    name = QString("%1 (%2)").arg(baseName).arg(suffix);
    suffix++;
  }

  // set up feature attributes
  FeatureAttributes attr;
  attr.count = 1;
  attr.typeOfFeature = type;
  attr.name = name;
  attr.group = nominal->getGroupName();

  // get measurement config
  QString elementConfigName = Settings::get(
      QString("defaultMeasurementConfig.%1")
          .arg(getElementTypeName(getElementTypeEnum(
              nominal->getFeatureWrapper()->getFeatureTypeString()))));
  MeasurementConfig mConfig =
      this->measurementConfigManager->getUserConfig(elementConfigName);

  // create the actual
  std::vector<oi::Guarded<FeatureWrapper> > created =
      this->job->addFeatures(attr);
  if (created.size() != 1 || created.front().isNull() ||
      created.front()->getGeometry().isNull()) {
    return oi::Guarded<FeatureWrapper>(nullptr);
  }
  oi::Guarded<FeatureWrapper> actual = created.front();

  // attach its fit function and measurement config
  std::vector<oi::Guarded<FeatureWrapper> > actuals;
  actuals.push_back(actual);
  sys::Function defaultFunction =
      PluginRepository::getDefaultFunction(type).value_or(sys::Function());
  this->addFunctionsAndMConfigs(actuals, mConfig,
                                defaultFunction.plugin().filePath(),
                                defaultFunction.name());

  // now that it has a fit attached it is read-only (D19) - record which
  // nominal it realises
  actual->getGeometry()->setRealises(nominal);

  return actual;
}

/*!
 * \brief Controller::addFunctionsAndMConfigs
 * \param actuals
 * \param mConfig
 * \param path
 * \param fName
 */
void Controller::addFunctionsAndMConfigs(
    const std::vector<oi::Guarded<FeatureWrapper> >& actuals,
    const MeasurementConfig& mConfig, const QString& path,
    const QString& fName) {
  // check job
  if (this->job.isNull()) {
    return;
  }

  foreach (const oi::Guarded<FeatureWrapper>& feature, actuals) {
    // check feature
    if (feature.isNull() || feature->getFeature().isNull()) {
      continue;
    }

    // load function plugin
    oi::Guarded<Function> function(nullptr);
    if (!path.isEmpty() && !fName.isEmpty()) {
      function = PluginLoader::loadFunctionPlugin(path, fName);
    }

    // assign function and measurement config to feature
    if (!function.isNull()) {
      feature->getFeature()->addFunction(function);
    }
    if (mConfig.isValid() && !feature->getGeometry().isNull()) {
      feature->getGeometry()->setMeasurementConfig(mConfig.getKey());
    }
  }
}

/*!
 * \brief Controller::connectDataExchanger
 */
void Controller::connectDataExchanger() {
  // messaging
  QObject::connect(&this->exchanger, &DataExchanger::sendMessage, this,
                   &Controller::log, Qt::AutoConnection);

  // nominal import
  QObject::connect(&this->exchanger, &DataExchanger::nominalImportFinished,
                   this, &Controller::nominalImportFinished,
                   Qt::AutoConnection);
  QObject::connect(
      &this->exchanger, &DataExchanger::updateNominalImportProgress, this,
      &Controller::nominalImportProgressUpdated, Qt::AutoConnection);

  // observation import
  QObject::connect(&this->exchanger, &DataExchanger::observationImportFinished,
                   this, &Controller::observationImportFinished,
                   Qt::AutoConnection);
  QObject::connect(
      &this->exchanger, &DataExchanger::updateObservationImportProgress, this,
      &Controller::observationImportProgressUpdated, Qt::AutoConnection);

  QObject::connect(this, &Controller::setExchangeParams, &this->exchanger,
                   &DataExchanger::setExchangeParams, Qt::AutoConnection);
}

/*!
 * \brief Controller::connectFeatureUpdater
 */
void Controller::connectFeatureUpdater() {}

#ifdef OI_WEBSOCKETSERVER_ENABLED
/*!
 * \brief Controller::connectRequestHandler
 */
void Controller::connectRequestHandler() {
  // sensor actions
  QObject::connect(&this->requestHandler, &OiRequestHandler::startAim, this,
                   &Controller::startAim, Qt::AutoConnection);
  QObject::connect(&this->requestHandler, &OiRequestHandler::startMeasurement,
                   this, &Controller::startMeasurement, Qt::AutoConnection);
  QObject::connect(&this->requestHandler, &OiRequestHandler::startReadingStream,
                   this, &Controller::startReadingStream, Qt::AutoConnection);
  QObject::connect(&this->requestHandler, &OiRequestHandler::stopReadingStream,
                   this, &Controller::stopReadingStream, Qt::AutoConnection);

  // connect streaming
  QObject::connect(
      this, &Controller::sensorActionStarted, &this->requestHandler,
      &OiRequestHandler::sensorActionStarted, Qt::QueuedConnection);
  QObject::connect(
      this, &Controller::sensorActionFinished, &this->requestHandler,
      &OiRequestHandler::sensorActionFinished, Qt::QueuedConnection);
  QObject::connect(this, &Controller::showClientMessage, &this->requestHandler,
                   &OiRequestHandler::log, Qt::QueuedConnection);
  QObject::connect(
      this, &Controller::activeFeatureChanged, &this->requestHandler,
      &OiRequestHandler::activeFeatureChanged, Qt::QueuedConnection);
  QObject::connect(
      this, &Controller::activeStationChanged, &this->requestHandler,
      &OiRequestHandler::activeStationChanged, Qt::QueuedConnection);
  QObject::connect(
      this, &Controller::activeCoordinateSystemChanged, &this->requestHandler,
      &OiRequestHandler::activeCoordinateSystemChanged, Qt::QueuedConnection);
  QObject::connect(this, &Controller::featureSetChanged, &this->requestHandler,
                   &OiRequestHandler::featureSetChanged, Qt::QueuedConnection);
  QObject::connect(
      this, &Controller::featureAttributesChanged, &this->requestHandler,
      &OiRequestHandler::featureAttributesChanged, Qt::QueuedConnection);
}
#endif

/*!
 * \brief Controller::connectToolPlugin
 * \param tool
 */
void Controller::connectToolPlugin(const QPointer<Tool>& tool) {
  if (tool.isNull()) {
    return;
  }

  // Tool -> Controller
  QObject::connect(tool.data(), &Tool::startConnect, this,
                   &Controller::startConnect, Qt::AutoConnection);
  QObject::connect(tool.data(), &Tool::startDisconnect, this,
                   &Controller::startDisconnect, Qt::AutoConnection);
  QObject::connect(tool.data(), &Tool::startMeasurement, this,
                   &Controller::startMeasurement, Qt::AutoConnection);
  QObject::connect(tool.data(), &Tool::startMove, this, &Controller::startMove,
                   Qt::AutoConnection);
  QObject::connect(tool.data(), &Tool::startAim, this, &Controller::startAim,
                   Qt::AutoConnection);
  QObject::connect(tool.data(), &Tool::startAimAndMeasure, this,
                   &Controller::startAimAndMeasure, Qt::AutoConnection);
  QObject::connect(tool.data(), &Tool::startToggleSight, this,
                   &Controller::startToggleSight, Qt::AutoConnection);
  QObject::connect(tool.data(), &Tool::startInitialize, this,
                   &Controller::startInitialize, Qt::AutoConnection);
  QObject::connect(tool.data(), &Tool::startHome, this, &Controller::startHome,
                   Qt::AutoConnection);
  QObject::connect(tool.data(), &Tool::startCompensation, this,
                   &Controller::startCompensation, Qt::AutoConnection);
  QObject::connect(tool.data(), &Tool::startChangeMotorState, this,
                   &Controller::startChangeMotorState, Qt::AutoConnection);
  QObject::connect(tool.data(), &Tool::startCustomAction, this,
                   &Controller::startCustomAction, Qt::AutoConnection);
  QObject::connect(tool.data(), &Tool::sendMessage, this, &Controller::log,
                   Qt::AutoConnection);

  // Controller -> Tool
  QObject::connect(this, &Controller::setCurrentJob, tool.data(),
                   &Tool::setCurrentJob, Qt::AutoConnection);
}

/*!
 * \brief Controller::showToolWidget
 * \param pluginName
 * \param toolName
 */
void Controller::showToolWidget(const QString& pluginName,
                                const QString& toolName) {
  // get a list of available tool plugins
  const QList<QPointer<Tool> >& tools = this->getAvailableTools();

  // search the list for the specified tool
  foreach (const QPointer<Tool>& tool, tools) {
    if (tool.isNull()) {
      continue;
    }

    if (tool->getMetaData().pluginName.compare(pluginName) == 0 &&
        tool->getMetaData().name.compare(toolName) == 0) {
      tool->setJob(this->job);
      tool->show();
      break;
    }
  }
}

void Controller::startSearch() {
  oi::Guarded<Station> activeStation = getConnectedActiveStation();
  if (activeStation.isNull()) {
    return;
  }

  // inform about start of sensor action
  emit this->sensorActionStarted("performing search...");

  activeStation->search();
}

void Controller::createTemplateFromJob() {
  // check job
  if (this->job.isNull()) {
    this->log("No job available", MessageTypes::eErrorMessage,
              MessageDestinations::eMessageBoxMessage);
    return;
  }

  this->job->createTemplateFromJob();
}

void Controller::createNewStation() {
  // check job
  if (this->job.isNull()) {
    this->log("No job available", MessageTypes::eErrorMessage,
              MessageDestinations::eMessageBoxMessage);
    return;
  }

  const int count = this->job->getStationsList().size();

  FeatureAttributes attributes;
  attributes.typeOfFeature = FeatureTypes::eStationFeature;
  attributes.name =
      QStringLiteral("STATION%1").arg(count + 1, 2, 10, QLatin1Char('0'));
  attributes.group = "00_Stations";
  attributes.count = 1;

  this->addFeatures(attributes);

  this->job->getStationsList()
      .back()
      ->getFeatureWrapper()
      ->getStation()
      ->setActiveStationState(true);
}

bool Controller::isStationBundled(int id) {
  const QString stationName =
      this->job->getFeatureById(id)->getFeature()->getFeatureName();

  foreach (oi::Guarded<TrafoParam> param,
           this->job->getTransformationParametersList()) {
    if (param->getStartSystem()->getIsBundleSystem() &&
        param->getDestinationSystem()->getIsStationSystem() &&
        param->getDestinationSystem()->getFeatureName() == stationName) {
      return param->getIsSolved() && param->getIsUsed();
    }
  }

  return false;
}

QList<QJsonObject> Controller::loadBundleTemplates() {
  QList<QJsonObject> templates;
  // get template path
  QString path;
  path =
      QString("%1%2").arg(qApp->applicationDirPath()).arg("/templates/bundle");

  // iterate over templates
  QDirIterator it(path, QDirIterator::NoIteratorFlags);
  while (it.hasNext()) {
    // load file
    QFile file(it.next());
    if (!file.open(QFile::ReadOnly)) {
      continue;
    }

    // save json template
    QJsonDocument bundleTemplate;
    QJsonParseError parseError;
    bundleTemplate = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
      continue;
    }
    templates.append(bundleTemplate.object());

    // close file
    file.close();
  }

  return templates;
}

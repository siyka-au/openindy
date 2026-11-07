#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <QFile>
#include <QFileDevice>
#include <QFileInfo>
#include <QMutex>
#include <QMutexLocker>
#include <QObject>
#include <QPointer>
#include <QSaveFile>
#include <QString>
#include <QThread>
#include <map>
#include <set>
#include <vector>

#include "calibratedconstellation.h"
#include "console.h"
#include "dataexchanger.h"
#include "exchangeParams.h"
#include "featureattributes.h"
#include "featuretablecolumnconfig.h"
#include "featureupdater.h"
#include "measurementconfigmanager.h"
#include "modelmanager.h"
#include "oijob.h"
#include "parameterdisplayconfig.h"
#include "pluginmanager.h"
#include "projectexchanger.h"
#include "qtexecutor.h"
#include "sensorconfigurationmanager.h"
#include "settings.h"
#include "trafoparamtablecolumnconfig.h"
#ifdef OI_WEBSOCKETSERVER_ENABLED
#include "oiwebsocketserver.h"
#endif
#include "controllersensoractions.h"
#include "projectconfig.h"

using namespace oi;

struct StationStatusData {
  QVariant id;
  QString name;
};

/*!
 * \brief The Controller class
 */
class Controller : public QObject, public ControllerSensorActions {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(Controller)

 public:
  explicit Controller(QObject* parent = nullptr);
  ~Controller() override;

  void init();

 public:
  // ###############################
  // getter to query available tools
  // ###############################

  [[nodiscard]] const QList<QPointer<Tool>>& getAvailableTools() const;

  bool hasProjectDigestChanged();

  void _startMeasurement(bool dummyPoint);

  bool isStationBundled(int id);

  QList<QJsonObject> loadBundleTemplates();

 private slots:
  void startAimAndMeasure() override;

 public slots:

  // #######################
  // tasks triggered by user
  // #######################

  // add or remove features
  void addFeatures(const FeatureAttributes& attributes);
  void removeFeatures(const std::set<int>& featureIds);
  oi::Guarded<FeatureWrapper> getActiveFeature();

  // change feature parameters
  void setNominalParameters(
      const int& featureId,
      const std::map<GeometryParameters, double>& parameters);
  void setTrafoParamParameters(
      const int& featureId,
      const std::map<TrafoParamParameters, double>& parameters);

  // recalculation
  void recalcActiveFeature();

  // UI verification checkpoint: generate a function-owned sub-geometry
  //(Stage 7h, D19) for the active feature - "check a box in a
  // relationship" per the user's own framing. No-arg, operates on the
  // active feature, same convention as startAim()/recalcActiveFeature().
  void generateOwnedNormalLine();
  void generateOwnedOriginPoint();

  // set sensor configuration for active station
  void setSensorConfig(const SensorConfiguration& sConfig, bool connectSensor);

  // remove the sensor of the active station
  void removeActiveStationSensor();

  // sensor configs edited
  void sensorConfigurationsEdited(const SensorConfigurationManager& manager);

  // active station's sensor config edited
  void sensorConfigurationUpdated(const SensorConfiguration& sConfig);

  // set measurement configuration for active feature
  void measurementConfigurationChanged(const MeasurementConfig& mConfig);
  void saveUserConfig(const MeasurementConfig& mConfig);

  // import or export features
  void importNominals(const ExchangeParams& params);
  void exportFeatures(const ExchangeParams& params);

  // add or remove observations
  void importObservations(const QString& filename);
  void removeObservations(const int& featureId);
  void removeObservationsById(const std::vector<int> selectedIds);
  void removeAllObservations();
  void enableObservations(const int& featureId);
  void disableObservations(const int& featureId);

  // set display configs
  void setFeatureTableColumnConfig(const FeatureTableColumnConfig& config);
  void setTrafoParamColumnConfig(const TrafoParamTableColumnConfig& config);
  void setObservationTableColumnConfig(
      const ObservationTableColumnConfig& config);
  void setReadingTableColumnConfig(const ReadingTableColumnConfig& config);
  void setParameterDisplayConfig(const ParameterDisplayConfig& config);

  // add or remove bundle system
  void addBundleSystem();
  void removeBundleSystem(const int& id);

  // load or calculate bundle
  QJsonObject getBundleTemplate();
  oi::Guarded<BundleAdjustment> getBundleAdjustment();
  void updateBundleAdjustment(const QJsonObject& param);
  void loadBundleTemplate(const QJsonObject& bundleTemplate);
  void runBundle();

  // save or load a job
  void saveProject();
  void saveProject(const QString& fileName);
  void loadProject(const QString& projectName, const QString& filePath);
  const oi::Guarded<OiJob>& createDefaultJob();

  // sensor actions
  void startConnect() override;
  void startDisconnect() override;
  void startMeasurement() override;
  void startMove(const Reading& reading) override;
  void startAim() override;
  void startToggleSight() override;
  void startInitialize() override;
  void startHome() override;
  void startCompensation() override;
  void startChangeMotorState() override;
  void startCustomAction(const QString& task) override;
  void startReadingStream(ReadingTypes streamFormat) override;
  void stopReadingStream() override;
  void finishMeasurement() override;
  void startSearch() override;

  // log messages to the specified destination
  void log(const QString& msg, const MessageTypes& msgType,
           const MessageDestinations& msgDest);

  // init configs from mainwindowSlot
  void initConfigs();

  // show tool widget
  void showToolWidget(const QString& pluginName, const QString& toolName);

  void createTemplateFromJob();

  void createNewStation();

 signals:

  void stationStatus(const StationStatusData& status);

  void sensorStatus(const SensorStatus& status, const QString& msg);

  // #################################################
  // signals to inform about current job state changes
  // #################################################

  // active feature changes
  void activeFeatureChanged();
  void activeStationChanged();
  void activeCoordinateSystemChanged();

  // feature(s) added or removed
  void featureSetChanged();
  void coordSystemSetChanged();
  void stationSetChanged();
  void trafoParamSetChanged();
  void geometrySetChanged();

  bool featureCreated(bool created);

  void setExchangeParams(ExchangeParams exchangeParams);

  // group(s) added or removed
  void availableGroupsChanged();
  void activeGroupChanged();

  // general feature attributes changed
  void featureAttributesChanged();

  // feature specific attributes changed
  void featureNameChanged(const int& featureId, const QString& oldName);
  void featureGroupChanged(const int& featureId, const QString& oldGroup);
  void featureCommentChanged(const int& featureId);
  void featureIsUpdatedChanged(const int& featureId);
  void featureIsSolvedChanged(const int& featureId);
  void featureFunctionsChanged(const int& featureId);
  void featureUsedForChanged(const int& featureId);
  void featurePreviouslyNeededChanged(const int& featureId);

  // geometry specific attributes changed
  void geometryIsCommonChanged(const int& featureId);
  void geometryRealisesChanged(const int& featureId);
  void geometryObservationsChanged(const int& featureId);
  void geometryNominalSystemChanged(const int& featureId);
  void geometryStatisticChanged(const int& featureId);
  void geometrySimulationDataChanged(const int& featureId);
  void geometryMeasurementConfigChanged(const int& featureId);

  // coordinate system specific attributes changed
  void systemObservationsChanged(const int& featureId, const int& obsId);
  void systemTrafoParamsChanged(const int& featureId);
  void systemsNominalsChanged(const int& featureId);

  // station specific attributes changed
  void stationSensorChanged(const int& featureId);

  // transformation parameter specific attributes changed
  void trafoParamParametersChanged(const int& featureId);
  void trafoParamSystemsChanged(const int& featureId);
  void trafoParamIsUsedChanged(const int& featureId);
  void trafoParamValidTimeChanged(const int& featureId);
  // void trafoParamIsMovementChanged(const int &featureId);

  // whole job instance changed
  void currentJobChanged();
  // set current job to other modules / tools
  void setCurrentJob(const oi::Guarded<OiJob>& job);

  // #################################
  // import export task status changes
  // #################################

  // nominal import
  void nominalImportStarted();
  void nominalImportProgressUpdated(const int& progress, const QString& msg);
  void nominalImportFinished(const bool& success);

  // observation import
  void observationImportStarted();
  void observationImportProgressUpdated(const int& progress,
                                        const QString& msg);
  void observationImportFinished(const bool& success);

  // ##############
  // sensor actions
  // ##############

  // sensor actions
  void sensorActionStarted(
      const QString& name,
      const SensorAction sensorAction = SensorAction::eSensorActionUndefind,
      const bool enableFinishButton = false);
  void sensorActionFinished(
      const bool& success, const QString& msg,
      const SensorAction sensorAction = SensorAction::eSensorActionUndefind);
  void measurementCompleted();
  void measurementDone(bool success);

  // #############
  // show messages
  // #############

  // messaging
  int showMessageBox(const QString& msg, const MessageTypes& msgType);
  void showStatusMessage(const QString& msg, const MessageTypes& msgType);
  void showClientMessage(const QString& msg, const MessageTypes& msgType);
  void showStatusSensor(const SensorStatus& code, const QString& msg);

  // #################
  // update status bar
  // #################

  void updateStatusBar();

  // ########################
  // trigger GUI interactions
  // ########################

  void saveAsTriggered();

  // ##############################
  // web socket server interactions
  // ##############################

  // start and stop server
#ifdef OI_WEBSOCKETSERVER_ENABLED
  void startWebSocketServer();
  void stopWebSocketServer();
#endif

  // error create trafo Param
  void requestMessageBoxTrafoParam();

 private slots:

  // ###################################
  // slots to react on job state changes
  // ###################################

  // active station changed
  void activeStationChangedCallback();

  // ################################
  // slots to react on sensor results
  // ################################

  void measurementFinished(const int& geomId,
                           const std::vector<oi::Guarded<Reading>>& readings);

 private:
  // ##############
  // helper methods
  // ##############

  // set OpenIndy job
  void setJob(const oi::Guarded<OiJob>& job);

  // init configs and plugins
  void initDisplayConfigs();
  void initConfigManager();
  void initToolPlugins();
  void initDefaults();

  // register meta types
  void registerMetaTypes();

  // start or stop OpenIndy server
#ifdef OI_WEBSOCKETSERVER_ENABLED
  void initServer();
  void startServer();
  void stopServer();
  void connectRequestHandler();
#endif

  // create feature helpers

  //"measure this nominal" (domain-model.md §4): always creates a new
  // actual referencing this nominal via Geometry::realises - D17
  // deliberately allows many actuals per nominal (re-inspection over
  // time), so this never reuses an existing one.
  oi::Guarded<FeatureWrapper> createActualForNominal(
      const oi::Guarded<Geometry>& nominal);
  void addFunctionsAndMConfigs(
      const std::vector<oi::Guarded<FeatureWrapper>>& actuals,
      const MeasurementConfig& mConfig, const QString& path,
      const QString& fName);

  // shared by generateOwnedNormalLine()/generateOwnedOriginPoint(): creates
  // a bare subType feature with no function attached, then attaches
  // functionName as its owned producer via OiJob::addOwnedSubGeometry.
  void generateOwnedSubGeometry(const FeatureTypes& subType,
                                const QString& functionName,
                                const QString& namePrefix);

  // ###############
  // set up connects
  // ###############

  // connect helper objects
  void connectDataExchanger();
  void connectFeatureUpdater();

  // connect tools
  void connectToolPlugin(const QPointer<Tool>& tool);

 private:
  // ##################
  // general attributes
  // ##################

  oi::Guarded<OiJob> job;

  // data exchanger
  DataExchanger exchanger;

  // feature updater
  FeatureUpdater featureUpdater;

  // delivers the sensor results of the job's stations on the UI thread
  QtExecutor executor;

  // config manager
  QPointer<SensorConfigurationManager> sensorConfigManager;
  QPointer<MeasurementConfigManager> measurementConfigManager;
  ProjectExchanger projectExchanger;

#ifdef OI_WEBSOCKETSERVER_ENABLED
  // thread and server instance
  QThread serverThread;
  QPointer<OiWebSocketServer> webSocketServer;

  // request handler for web socket requests
  OiRequestHandler requestHandler;
#endif

  // tool plugins
  QList<QPointer<Tool>> toolPlugins;

  // synchronize saveProject calls
  QMutex saveProjectMutex;

  oi::Guarded<Station> getConnectedActiveStation();

  oi::Guarded<Station> getActiveStation();
};

#endif  // CONTROLLER_H

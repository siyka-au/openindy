#ifndef OIREQUESTHANDLER_H
#define OIREQUESTHANDLER_H

#include <QMap>
#include <QObject>
#include <QPointer>
#include <QThread>
#include <QVariantMap>

#include "featuresorter.h"
#include "featureupdater.h"
#include "featurewrapper.h"
#include "measurementconfigmanager.h"
#include "oirequestresponse.h"
#include "projectexchanger.h"
#include "sensorconfigurationmanager.h"
#include "station.h"
#include "trafocontroller.h"

/*!
 * \brief The WatchWindowTask struct
 */
struct WatchWindowTask {
  bool taskInProgress{};
  ReadingTypes type;
  OiRequestResponse request;
};

namespace openindy::v1 {
class Request;
class Response;
class RealTimeReading;
class Notification;
}  // namespace openindy::v1

/*!
 * \brief The OiRequestHandler class
 * Receives remote-control requests (a serialized openindy.v1.Envelope, see
 * proto/openindy/network.proto) and emits the corresponding response and the
 * server-pushed notifications as serialized Envelopes
 */
class OiRequestHandler : public QObject {
  Q_OBJECT

 public:
  explicit OiRequestHandler(QObject* parent = nullptr);

  // ######################
  // get or set current job
  // ######################

  [[nodiscard]] const QPointer<OiJob>& getCurrentJob() const;
  void setCurrentJob(const QPointer<OiJob>& job);

  // ##########################
  // get or set config managers
  // ##########################

  [[nodiscard]] const QPointer<SensorConfigurationManager>&
  getSensorConfigManager() const;
  void setSensorConfigManager(
      const QPointer<SensorConfigurationManager>& sensorConfigManager);

  [[nodiscard]] const QPointer<MeasurementConfigManager>&
  getMeasurementConfigManager() const;
  void setMeasurementConfigManager(
      const QPointer<MeasurementConfigManager>& measurementConfigManager);

 public slots:

  // #######################################
  // receive requests from web socket server
  // #######################################

  bool receiveRequest(OiRequestResponse request);

  // ##################################
  // sensor actions started or finished
  // ##################################

  void sensorActionStarted(const QString& name);
  void sensorActionFinished(const bool& success, const QString& msg);

  // #########
  // messaging
  // #########

  void log(const QString& msg, const MessageTypes& msgType);

  // #######################
  // sensor real time stream
  // #######################

  void realTimeReading(const QVariantMap& reading);

  // #################
  // job state changes
  // #################

  // active feature changes
  void activeFeatureChanged();
  void activeStationChanged();
  void activeCoordinateSystemChanged();

  // feature(s) added or removed
  void featureSetChanged();

  // general feature attributes changed
  void featureAttributesChanged();

 signals:

  // ##################################
  // send response to web socket server
  // ##################################

  bool sendResponse(OiRequestResponse request);

  // ######################
  // trigger sensor actions
  // ######################

  void startAim();
  void startMeasurement();
  void startReadingStream(ReadingTypes type);
  void stopReadingStream();

 private:
  // ##############
  // helper methods
  // ##############

  // request processing: each fills the response payload and returns the
  // error code of the request
  using ErrorCode = OiRequestResponse::ErrorCode;
  using Request = openindy::v1::Request;
  using Response = openindy::v1::Response;

  ErrorCode getProject(const Request& request, Response& response);
  ErrorCode getActiveFeature(const Request& request, Response& response);
  ErrorCode setActiveFeature(const Request& request, Response& response);
  ErrorCode getActiveStation(const Request& request, Response& response);
  ErrorCode setActiveStation(const Request& request, Response& response);
  ErrorCode getActiveCoordinateSystem(const Request& request,
                                      Response& response);
  ErrorCode setActiveCoordinateSystem(const Request& request,
                                      Response& response);
  ErrorCode aim(const Request& request, Response& response);
  ErrorCode measure(const Request& request, Response& response);
  ErrorCode startWatchwindow(const Request& request, Response& response,
                             const OiRequestResponse& carrier);
  ErrorCode stopWatchwindow(const Request& request, Response& response);
  ErrorCode getFeatures(const Request& request, Response& response);
  ErrorCode addFeatures(const Request& request, Response& response);
  ErrorCode getObservations(const Request& request, Response& response);
  ErrorCode removeObservations(const Request& request, Response& response);
  ErrorCode getParameters(const Request& request, Response& response);
  ErrorCode getMeasurementConfigs(const Request& request, Response& response);
  ErrorCode getMeasurementConfig(const Request& request, Response& response);
  ErrorCode setMeasurementConfig(const Request& request, Response& response);
  ErrorCode getCoordinateSystems(const Request& request, Response& response);

  // the parameters of a feature as sorted name/value pairs
  [[nodiscard]] static QMap<QString, double> parametersOf(
      const QPointer<FeatureWrapper>& feature);

  // send an error response (no payload)
  void sendErrorMessage(OiRequestResponse request, int requestId,
                        OiRequestResponse::RequestType type,
                        OiRequestResponse::ErrorCode error);

  // send a server-pushed notification to the given requester (-1: everyone)
  void sendNotification(OiRequestResponse::RequestType type,
                        const openindy::v1::Notification& notification,
                        int requesterId = -1);

  // set up watch window data
  bool buildWatchWindowMessage(openindy::v1::RealTimeReading& reading,
                               const QVariantMap& streamData);

 private slots:

  // ##################################
  // watch window and measure callbacks
  // ##################################

  // void receiveWatchWindowData(const QVariantMap &data);
  // void measurementFinished(const bool &success);

 private:
  // #################
  // helper attributes
  // #################

  // config manager
  QPointer<SensorConfigurationManager> sensorConfigManager;
  QPointer<MeasurementConfigManager> measurementConfigManager;
  ProjectExchanger projectExchanger;

  // job
  QPointer<OiJob> currentJob;

  // feature sorter
  FeatureSorter sorter;

  // watch window task
  WatchWindowTask watchWindowTask;
};

#endif  // OIREQUESTHANDLER_H

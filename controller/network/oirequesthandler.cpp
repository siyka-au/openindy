#include "oirequesthandler.h"

#include <math.h>

#include <algorithm>
#include <map>
#include <vector>

#include "containers.h"
#include "convert.h"
#include "network_convert.h"
#include "openindy/network.pb.h"
#include "qtconvert.h"
#include "qtstationadapter.h"

namespace oipb = openindy::v1;

namespace {

// The feature types a client may create, in the order the first protocol
// versions numbered them. Group, point group and level are not creatable
// over the network.
std::optional<FeatureTypes> creatableType(oipb::FeatureKind kind) {
  const auto type = oi::codec::fromProto(kind);
  if (!type.has_value()) {
    return std::nullopt;
  }
  switch (*type) {
    case FeatureTypes::eUndefinedFeature:
    case FeatureTypes::eLevelFeature:
    case FeatureTypes::eGroupFeature:
    case FeatureTypes::ePointGroupFeature:
      return std::nullopt;
    default:
      return type;
  }
}

OiRequestResponse::RequestType requestTypeOf(
    oipb::Request::PayloadCase payload) {
  using Case = oipb::Request;
  using Type = OiRequestResponse::RequestType;
  switch (payload) {
    case Case::kGetProject:
      return Type::eGetProject;
    case Case::kGetActiveFeature:
      return Type::eGetActiveFeature;
    case Case::kSetActiveFeature:
      return Type::eSetActiveFeature;
    case Case::kGetActiveStation:
      return Type::eGetActiveStation;
    case Case::kSetActiveStation:
      return Type::eSetActiveStation;
    case Case::kGetActiveCoordinateSystem:
      return Type::eGetActiveCoordinateSystem;
    case Case::kSetActiveCoordinateSystem:
      return Type::eSetActiveCoordinateSystem;
    case Case::kAim:
      return Type::eAim;
    case Case::kMeasure:
      return Type::eMeasure;
    case Case::kStartWatchwindow:
      return Type::eStartWatchwindow;
    case Case::kStopWatchwindow:
      return Type::eStopWatchwindow;
    case Case::kTool:
      return Type::eOiToolRequest;
    case Case::kGetFeatures:
      return Type::eGetFeatures;
    case Case::kAddFeatures:
      return Type::eAddFeatures;
    case Case::kGetObservations:
      return Type::eGetObservations;
    case Case::kRemoveObservations:
      return Type::eRemoveObservations;
    case Case::kGetParameters:
      return Type::eGetParameters;
    case Case::kGetMeasurementConfigs:
      return Type::eGetMeasurementConfigs;
    case Case::kGetMeasurementConfig:
      return Type::eGetMeasurementConfig;
    case Case::kSetMeasurementConfig:
      return Type::eSetMeasurementConfig;
    case Case::kGetCoordinateSystems:
      return Type::eGetCoordinateSystems;
    case Case::PAYLOAD_NOT_SET:
      break;
  }
  return Type::eUnknownRequest;
}

std::string serialize(const oipb::Envelope& envelope) {
  return envelope.SerializeAsString();
}

int idOf(const Feature* feature) {
  return feature != nullptr ? feature->getId() : 0;
}

oipb::MeasurementConfigEntry configEntry(const MeasurementConfig& config,
                                         bool saved) {
  oipb::MeasurementConfigEntry entry;
  *entry.mutable_config() = oi::codec::toProto(config);
  entry.set_saved(saved);
  return entry;
}

}  // namespace

OiRequestHandler::OiRequestHandler(QObject* parent) {
  // initially no watch window task in process
  this->watchWindowTask.taskInProgress = false;
}

/*!
 * \brief OiRequestHandler::getCurrentJob
 * \return
 */
const oi::Guarded<OiJob>& OiRequestHandler::getCurrentJob() const {
  return this->currentJob;
}

/*!
 * \brief OiRequestHandler::setCurrentJob
 * \param job
 */
void OiRequestHandler::setCurrentJob(const oi::Guarded<OiJob>& job) {
  if (!job.isNull()) {
    this->currentJob = job;
    sorter.setCurrentJob(job);
  }
}

/*!
 * \brief OiRequestHandler::getSensorConfigManager
 * \return
 */
const QPointer<SensorConfigurationManager>&
OiRequestHandler::getSensorConfigManager() const {
  return this->sensorConfigManager;
}

/*!
 * \brief OiRequestHandler::setSensorConfigManager
 * \param sensorConfigManager
 */
void OiRequestHandler::setSensorConfigManager(
    const QPointer<SensorConfigurationManager>& sensorConfigManager) {
  this->sensorConfigManager = sensorConfigManager;
}

/*!
 * \brief OiRequestHandler::getMeasurementConfigManager
 * \return
 */
const QPointer<MeasurementConfigManager>&
OiRequestHandler::getMeasurementConfigManager() const {
  return this->measurementConfigManager;
}

/*!
 * \brief OiRequestHandler::setMeasurementConfigManager
 * \param measurementConfigManager
 */
void OiRequestHandler::setMeasurementConfigManager(
    const QPointer<MeasurementConfigManager>& measurementConfigManager) {
  this->projectExchanger.setMeasurementConfigManager(measurementConfigManager);
  this->measurementConfigManager = measurementConfigManager;
}

/*!
 * \brief OiRequestHandler::receiveRequest
 * Parses a serialized request envelope and calls the corresponding method to
 * do the requested task. Whatever the input, exactly one response is sent
 * (except for tool requests, which are not answered).
 * \param request
 * \return
 */
bool OiRequestHandler::receiveRequest(OiRequestResponse request) {
  using Type = OiRequestResponse::RequestType;
  using Error = OiRequestResponse::ErrorCode;

  // check request: it must be an envelope holding a request
  oipb::Envelope envelope;
  if (!envelope.ParseFromArray(request.request.data(),
                               static_cast<int>(request.request.size())) ||
      envelope.content_case() != oipb::Envelope::kRequest) {
    this->sendErrorMessage(request, 0, Type::eUnknownRequest,
                           Error::eWrongFormat);
    return false;
  }
  const int requestId = envelope.request_id();
  const oipb::Request& message = envelope.request();

  // a request without a payload is malformed; one with a payload this version
  // does not know (a field from a newer protocol) is an unknown request
  const Type type = requestTypeOf(message.payload_case());
  if (type == Type::eUnknownRequest &&
      message.GetReflection()->GetUnknownFields(message).empty()) {
    this->sendErrorMessage(request, requestId, type, Error::eWrongFormat);
    return false;
  }

  // check current job
  if (this->currentJob.isNull()) {
    this->sendErrorMessage(request, requestId, type, Error::eNoJob);
    return false;
  }

  // tool requests are not answered
  if (type == Type::eOiToolRequest) {
    return true;
  }

  // perform the task defined by type
  oipb::Response response;
  response.set_request(oi::codec::toProto(type));
  Error error = Error::eNoError;
  switch (message.payload_case()) {
    case oipb::Request::kGetProject:
      error = this->getProject(message, response);
      break;
    case oipb::Request::kGetActiveFeature:
      error = this->getActiveFeature(message, response);
      break;
    case oipb::Request::kSetActiveFeature:
      error = this->setActiveFeature(message, response);
      break;
    case oipb::Request::kGetActiveStation:
      error = this->getActiveStation(message, response);
      break;
    case oipb::Request::kSetActiveStation:
      error = this->setActiveStation(message, response);
      break;
    case oipb::Request::kGetActiveCoordinateSystem:
      error = this->getActiveCoordinateSystem(message, response);
      break;
    case oipb::Request::kSetActiveCoordinateSystem:
      error = this->setActiveCoordinateSystem(message, response);
      break;
    case oipb::Request::kAim:
      error = this->aim(message, response);
      break;
    case oipb::Request::kMeasure:
      error = this->measure(message, response);
      break;
    case oipb::Request::kStartWatchwindow:
      error = this->startWatchwindow(message, response, request);
      break;
    case oipb::Request::kStopWatchwindow:
      error = this->stopWatchwindow(message, response);
      break;
    case oipb::Request::kGetFeatures:
      error = this->getFeatures(message, response);
      break;
    case oipb::Request::kAddFeatures:
      error = this->addFeatures(message, response);
      break;
    case oipb::Request::kGetObservations:
      error = this->getObservations(message, response);
      break;
    case oipb::Request::kRemoveObservations:
      error = this->removeObservations(message, response);
      break;
    case oipb::Request::kGetParameters:
      error = this->getParameters(message, response);
      break;
    case oipb::Request::kGetMeasurementConfigs:
      error = this->getMeasurementConfigs(message, response);
      break;
    case oipb::Request::kGetMeasurementConfig:
      error = this->getMeasurementConfig(message, response);
      break;
    case oipb::Request::kSetMeasurementConfig:
      error = this->setMeasurementConfig(message, response);
      break;
    case oipb::Request::kGetCoordinateSystems:
      error = this->getCoordinateSystems(message, response);
      break;
    case oipb::Request::kTool:
    case oipb::Request::PAYLOAD_NOT_SET:
      this->sendErrorMessage(request, requestId, type,
                             Error::eUnknownRequestType);
      return false;
  }

  // answer
  response.set_error(oi::codec::toProto(error));
  oipb::Envelope answer;
  answer.set_request_id(requestId);
  *answer.mutable_response() = std::move(response);
  request.myRequestType = type;
  request.errorCode = error;
  request.response = serialize(answer);
  emit this->sendResponse(request);
  return true;
}

/*!
 * \brief OiRequestHandler::sendNotification
 * Sends a notification to one requester or, with -1, to everybody
 */
void OiRequestHandler::sendNotification(
    OiRequestResponse::RequestType type,
    const openindy::v1::Notification& notification, int requesterId) {
  oipb::Envelope envelope;
  *envelope.mutable_notification() = notification;

  OiRequestResponse request;
  request.requesterId = requesterId;
  request.myRequestType = type;
  request.response = serialize(envelope);
  emit this->sendResponse(request);
}

/*!
 * \brief OiRequestHandler::sensorActionStarted
 * \param name
 */
void OiRequestHandler::sensorActionStarted(const QString& name) {
  oipb::Notification notification;
  notification.mutable_sensor_action_started()->set_name(name.toStdString());
  this->sendNotification(OiRequestResponse::RequestType::eSensorActionStarted,
                         notification);
}

/*!
 * \brief OiRequestHandler::sensorActionFinished
 * \param success
 * \param msg
 */
void OiRequestHandler::sensorActionFinished(const bool& success,
                                            const QString& msg) {
  oipb::Notification notification;
  auto* finished = notification.mutable_sensor_action_finished();
  finished->set_success(success);
  finished->set_message(msg.toStdString());
  this->sendNotification(OiRequestResponse::RequestType::eSensorActionFinished,
                         notification);
}

/*!
 * \brief OiRequestHandler::log
 * \param msg
 * \param msgType
 */
void OiRequestHandler::log(const QString& msg, const MessageTypes& msgType) {
  oipb::Notification notification;
  auto* box = notification.mutable_message_box();
  box->set_text(msg.toStdString());
  box->set_kind(oi::codec::toProto(msgType));
  this->sendNotification(OiRequestResponse::RequestType::eMessageBox,
                         notification);
}

/*!
 * \brief OiRequestHandler::realTimeReading
 * \param reading
 */
void OiRequestHandler::realTimeReading(const QVariantMap& reading) {
  // check watch window task
  if (!this->watchWindowTask.taskInProgress) {
    return;
  }

  // set up real time data
  oipb::Notification notification;
  if (!this->buildWatchWindowMessage(*notification.mutable_real_time_reading(),
                                     reading)) {
    return;
  }

  // only the client that started the watch window gets the readings
  this->sendNotification(OiRequestResponse::RequestType::eRealTimeReading,
                         notification,
                         this->watchWindowTask.request.requesterId);
}

/*!
 * \brief OiRequestHandler::activeFeatureChanged
 */
void OiRequestHandler::activeFeatureChanged() {
  oipb::Notification notification;
  notification.mutable_active_feature_changed();
  this->sendNotification(OiRequestResponse::RequestType::eActiveFeatureChanged,
                         notification);
}

/*!
 * \brief OiRequestHandler::activeStationChanged
 */
void OiRequestHandler::activeStationChanged() {
  oipb::Notification notification;
  notification.mutable_active_station_changed();
  this->sendNotification(OiRequestResponse::RequestType::eActiveStationChanged,
                         notification);
}

/*!
 * \brief OiRequestHandler::activeCoordinateSystemChanged
 */
void OiRequestHandler::activeCoordinateSystemChanged() {
  oipb::Notification notification;
  notification.mutable_active_coordinate_system_changed();
  this->sendNotification(
      OiRequestResponse::RequestType::eActiveCoordinateSystemChanged,
      notification);
}

/*!
 * \brief OiRequestHandler::featureSetChanged
 */
void OiRequestHandler::featureSetChanged() {
  oipb::Notification notification;
  notification.mutable_feature_set_changed();
  this->sendNotification(OiRequestResponse::RequestType::eFeatureSetChanged,
                         notification);
}

/*!
 * \brief OiRequestHandler::featureAttributesChanged
 */
void OiRequestHandler::featureAttributesChanged() {
  oipb::Notification notification;
  notification.mutable_feature_attributes_changed();
  this->sendNotification(
      OiRequestResponse::RequestType::eFeatureAttributesChanged, notification);
}

/*!
 * \brief OiRequestHandler::getProject
 */
OiRequestResponse::ErrorCode OiRequestHandler::getProject(
    const Request& /*request*/, Response& response) {
  const QByteArray project = projectExchanger.saveProject(this->currentJob);
  response.mutable_get_project()->set_project(
      project.constData(), static_cast<size_t>(project.size()));
  return ErrorCode::eNoError;
}

/*!
 * \brief OiRequestHandler::getActiveFeature
 */
OiRequestResponse::ErrorCode OiRequestHandler::getActiveFeature(
    const Request& /*request*/, Response& response) {
  auto* payload = response.mutable_get_active_feature();
  if (!this->currentJob->getActiveFeature().isNull() &&
      !this->currentJob->getActiveFeature()->getFeature().isNull()) {
    payload->set_id(
        this->currentJob->getActiveFeature()->getFeature()->getId());
    return ErrorCode::eNoError;
  }
  return ErrorCode::eNoActiveFeature;
}

/*!
 * \brief OiRequestHandler::setActiveFeature
 */
OiRequestResponse::ErrorCode OiRequestHandler::setActiveFeature(
    const Request& request, Response& response) {
  ErrorCode errorCode = ErrorCode::eNoError;

  // set the active feature
  const auto& message = request.set_active_feature();
  if (message.has_id()) {
    oi::Guarded<FeatureWrapper> myFeature =
        this->currentJob->getFeatureById(message.id());
    if (!myFeature.isNull() && !myFeature->getFeature().isNull()) {
      myFeature->getFeature()->setActiveFeatureState(true);
    } else {
      errorCode = ErrorCode::eNoFeatureWithId;
    }
  } else {
    errorCode = ErrorCode::eWrongFormat;
  }

  // report the new active feature
  auto* payload = response.mutable_set_active_feature();
  if (!this->currentJob->getActiveFeature().isNull() &&
      !this->currentJob->getActiveFeature()->getFeature().isNull()) {
    payload->set_id(
        this->currentJob->getActiveFeature()->getFeature()->getId());
  }
  return errorCode;
}

/*!
 * \brief OiRequestHandler::getActiveStation
 */
OiRequestResponse::ErrorCode OiRequestHandler::getActiveStation(
    const Request& /*request*/, Response& response) {
  auto* payload = response.mutable_get_active_station();
  if (!this->currentJob->getActiveStation().isNull()) {
    payload->set_id(this->currentJob->getActiveStation()->getId());
    return ErrorCode::eNoError;
  }
  return ErrorCode::eNoActiveStation;
}

/*!
 * \brief OiRequestHandler::setActiveStation
 */
OiRequestResponse::ErrorCode OiRequestHandler::setActiveStation(
    const Request& request, Response& response) {
  ErrorCode errorCode = ErrorCode::eNoError;

  // set the active station
  const auto& message = request.set_active_station();
  if (message.has_id()) {
    oi::Guarded<FeatureWrapper> myFeature =
        this->currentJob->getFeatureById(message.id());
    if (!myFeature.isNull() && !myFeature->getStation().isNull()) {
      myFeature->getStation()->setActiveStationState(true);
    } else {
      errorCode = ErrorCode::eNoFeatureWithId;
    }
  } else {
    errorCode = ErrorCode::eWrongFormat;
  }

  // report the new active station
  auto* payload = response.mutable_set_active_station();
  if (!this->currentJob->getActiveStation().isNull()) {
    payload->set_id(this->currentJob->getActiveStation()->getId());
  }
  return errorCode;
}

/*!
 * \brief OiRequestHandler::getActiveCoordinateSystem
 */
OiRequestResponse::ErrorCode OiRequestHandler::getActiveCoordinateSystem(
    const Request& /*request*/, Response& response) {
  auto* payload = response.mutable_get_active_coordinate_system();
  if (!this->currentJob->getActiveCoordinateSystem().isNull()) {
    payload->set_id(this->currentJob->getActiveCoordinateSystem()->getId());
    return ErrorCode::eNoError;
  }
  return ErrorCode::eNoActiveCoordinateSystem;
}

/*!
 * \brief OiRequestHandler::setActiveCoordinateSystem
 */
OiRequestResponse::ErrorCode OiRequestHandler::setActiveCoordinateSystem(
    const Request& request, Response& response) {
  ErrorCode errorCode = ErrorCode::eNoError;

  // set the active coordinate system
  const auto& message = request.set_active_coordinate_system();
  if (message.has_id()) {
    oi::Guarded<FeatureWrapper> myFeature =
        this->currentJob->getFeatureById(message.id());
    if (!myFeature.isNull() && !myFeature->getCoordinateSystem().isNull()) {
      myFeature->getCoordinateSystem()->setActiveCoordinateSystemState(true);
    } else {
      errorCode = ErrorCode::eNoFeatureWithId;
    }
  } else {
    errorCode = ErrorCode::eWrongFormat;
  }

  // report the new active coordinate system
  auto* payload = response.mutable_set_active_coordinate_system();
  if (!this->currentJob->getActiveCoordinateSystem().isNull()) {
    payload->set_id(this->currentJob->getActiveCoordinateSystem()->getId());
  }
  return errorCode;
}

/*!
 * \brief OiRequestHandler::aim
 */
OiRequestResponse::ErrorCode OiRequestHandler::aim(const Request& request,
                                                   Response& response) {
  // get feature id
  const auto& message = request.aim();
  if (!message.has_id()) {
    return ErrorCode::eWrongFormat;
  }

  // check if active station is valid
  if (this->currentJob->getActiveStation().isNull()) {
    return ErrorCode::eNoActiveStation;
  }

  // check if there is a sensor connected
  if (!this->currentJob->getActiveStation()->getIsSensorConnected()) {
    return ErrorCode::eNoSensorConnected;
  }

  // check if active coordinate system is valid
  if (this->currentJob->getActiveCoordinateSystem().isNull()) {
    return ErrorCode::eNoActiveCoordinateSystem;
  }

  // set the active feature
  oi::Guarded<FeatureWrapper> feature =
      this->currentJob->getFeatureById(message.id());
  if (feature.isNull() || feature->getGeometry().isNull()) {
    return ErrorCode::eNoFeatureWithId;
  }
  feature->getGeometry()->setActiveFeatureState(true);

  // start aiming the active feature
  emit this->startAim();

  response.mutable_aim();
  return ErrorCode::eNoError;
}

/*!
 * \brief OiRequestHandler::measure
 */
OiRequestResponse::ErrorCode OiRequestHandler::measure(const Request& request,
                                                       Response& response) {
  // get feature id
  const auto& message = request.measure();
  if (!message.has_id()) {
    return ErrorCode::eWrongFormat;
  }

  // check if active station is valid
  if (this->currentJob->getActiveStation().isNull()) {
    return ErrorCode::eNoActiveStation;
  }

  // check if there is a sensor connected
  if (!this->currentJob->getActiveStation()->getIsSensorConnected()) {
    return ErrorCode::eNoSensorConnected;
  }

  // check if active coordinate system is valid
  if (this->currentJob->getActiveCoordinateSystem().isNull()) {
    return ErrorCode::eNoActiveCoordinateSystem;
  }

  // set the active feature
  oi::Guarded<FeatureWrapper> feature =
      this->currentJob->getFeatureById(message.id());
  if (feature.isNull() || feature->getGeometry().isNull()) {
    return ErrorCode::eNoFeatureWithId;
  }
  feature->getGeometry()->setActiveFeatureState(true);

  // start measurement
  emit this->startMeasurement();

  response.mutable_measure();
  return ErrorCode::eNoError;
}

/*!
 * \brief OiRequestHandler::startWatchwindow
 */
OiRequestResponse::ErrorCode OiRequestHandler::startWatchwindow(
    const Request& request, Response& response,
    const OiRequestResponse& carrier) {
  // only one watch window task should be open at once
  if (this->watchWindowTask.taskInProgress) {
    return ErrorCode::eTaskInProcess;
  }

  // get requested reading type
  const auto myReadingType =
      oi::codec::fromProto(request.start_watchwindow().reading());
  if (!myReadingType.has_value()) {
    return ErrorCode::eWrongFormat;
  }

  // check active feature
  if (this->currentJob->getActiveFeature().isNull() ||
      this->currentJob->getActiveFeature()->getFeature().isNull()) {
    return ErrorCode::eNoActiveFeature;
  }

  // get and check active station
  oi::Guarded<Station> activeStation = this->currentJob->getActiveStation();
  if (activeStation.isNull()) {
    return ErrorCode::eNoActiveStation;
  }

  // check sensor
  if (!activeStation->getIsSensorConnected()) {
    return ErrorCode::eNoSensorConnected;
  }

  // connect the reading stream to the request handler
  QObject::connect(QtStationAdapter::of(activeStation),
                   &QtStationAdapter::realTimeReading, this,
                   &OiRequestHandler::realTimeReading, Qt::AutoConnection);

  // start watch window
  emit this->startReadingStream(*myReadingType);

  // save active watch window task
  this->watchWindowTask.taskInProgress = true;
  this->watchWindowTask.type = *myReadingType;
  this->watchWindowTask.request = carrier;

  response.mutable_start_watchwindow();
  return ErrorCode::eNoError;
}

/*!
 * \brief OiRequestHandler::stopWatchwindow
 */
OiRequestResponse::ErrorCode OiRequestHandler::stopWatchwindow(
    const Request& /*request*/, Response& response) {
  // if no task is in process, no task has to be stopped
  if (!this->watchWindowTask.taskInProgress) {
    return ErrorCode::eNoTask;
  }

  // get and check active station
  oi::Guarded<Station> activeStation = this->currentJob->getActiveStation();
  if (activeStation.isNull()) {
    return ErrorCode::eNoActiveStation;
  }

  // disconnect the reading stream from the request handler
  QObject::disconnect(QtStationAdapter::of(activeStation),
                      &QtStationAdapter::realTimeReading, this,
                      &OiRequestHandler::realTimeReading);

  // stop watch window
  emit this->stopReadingStream();

  // reset active watch window task
  this->watchWindowTask.taskInProgress = false;
  this->watchWindowTask.request = OiRequestResponse();

  response.mutable_stop_watchwindow();
  return ErrorCode::eNoError;
}

/*!
 * \brief OiRequestHandler::getFeatures
 */
OiRequestResponse::ErrorCode OiRequestHandler::getFeatures(
    const Request& /*request*/, Response& response) {
  // get a list of all features
  std::vector<oi::Guarded<FeatureWrapper> > features =
      this->currentJob->getFeaturesList();

  // sort the list according to the current sorting mode
  std::sort(features.begin(), features.end(), this->sorter);

  // describe the features
  auto* payload = response.mutable_get_features();
  foreach (const oi::Guarded<FeatureWrapper>& feature, features) {
    // check feature
    if (feature.isNull() || feature->getFeature().isNull()) {
      continue;
    }

    auto* summary = payload->add_features();
    summary->set_id(feature->getFeature()->getId());
    summary->set_kind(oi::codec::toProto(feature->getFeatureTypeEnum()));
    summary->set_name(feature->getFeature()->getFeatureName());
    summary->set_group(feature->getFeature()->getGroupName());
    summary->set_solved(feature->getFeature()->getIsSolved());
    summary->set_nominal(
        !(feature->getGeometry().isNull() ||
          feature->getGeometry()->getOwnerDescription().has_value()));
  }
  return ErrorCode::eNoError;
}

/*!
 * \brief OiRequestHandler::addFeatures
 */
OiRequestResponse::ErrorCode OiRequestHandler::addFeatures(
    const Request& request, Response& response) {
  // check measurement config manager
  if (this->measurementConfigManager.isNull()) {
    return ErrorCode::eNoMeasurementConfigManager;
  }

  // check request
  const auto& message = request.add_features();
  const auto type = creatableType(message.kind());
  if (!message.has_name() || !message.has_group() || !message.has_count() ||
      !type.has_value()) {
    return ErrorCode::eWrongFormat;
  }

  // set up feature attributes
  FeatureAttributes attr;
  attr.typeOfFeature = *type;
  attr.name = message.name();
  attr.group = message.group();
  attr.count = message.count();
  // D17: isActual/isNominal are no longer settable attributes - whether
  // a created geometry ends up nominal or actual now falls out of the
  // producer function attached to it, which this legacy network request
  // never specified in the first place
  if (message.has_nominal_system()) {
    attr.nominalSystem = message.nominal_system();
  }
  if (message.has_measurement_config()) {
    attr.measurementConfigName = message.measurement_config();
  }

  // add features
  std::vector<oi::Guarded<FeatureWrapper> > features =
      this->currentJob->addFeatures(attr);

  // get and check measurement config
  MeasurementConfig mConfig = this->measurementConfigManager->getUserConfig(
      toQString(attr.measurementConfigName));
  if (!mConfig.isValid()) {
    mConfig = this->measurementConfigManager->getProjectConfig(
        toQString(attr.measurementConfigName));
  }

  // pass measurement config to features
  foreach (const oi::Guarded<FeatureWrapper>& feature, features) {
    if (!feature.isNull() && !feature->getGeometry().isNull() &&
        feature->getGeometry()->getOwnerDescription().has_value()) {
      feature->getGeometry()->setMeasurementConfig(mConfig.getKey());
    }
  }

  response.mutable_add_features();
  return ErrorCode::eNoError;
}

/*!
 * \brief OiRequestHandler::getObservations
 */
OiRequestResponse::ErrorCode OiRequestHandler::getObservations(
    const Request& request, Response& response) {
  // check request
  const auto& message = request.get_observations();
  if (!message.has_id()) {
    return ErrorCode::eWrongFormat;
  }

  // get and check feature by id
  oi::Guarded<FeatureWrapper> feature =
      this->currentJob->getFeatureById(message.id());
  if (feature.isNull() || feature->getGeometry().isNull()) {
    return ErrorCode::eNoFeatureWithId;
  }

  auto* payload = response.mutable_get_observations();
  payload->set_id(message.id());

  // describe the observations
  foreach (const oi::Guarded<Observation>& obs,
           feature->getGeometry()->getObservations()) {
    // check observation
    if (obs.isNull()) {
      continue;
    }

    auto* observation = payload->add_observations();
    observation->set_id(obs->getId());
    observation->mutable_xyz()->set_x(obs->getXYZ().getAt(0));
    observation->mutable_xyz()->set_y(obs->getXYZ().getAt(1));
    observation->mutable_xyz()->set_z(obs->getXYZ().getAt(2));
    observation->set_valid(obs->getIsValid());

    // add deviations and use state
    if (feature->getGeometry()->getFunctions().size() > 0 &&
        !feature->getGeometry()->getFunctions().at(0).isNull()) {
      // get function and statistic
      oi::Guarded<Function> function =
          feature->getGeometry()->getFunctions().at(0);
      const Statistic& statistic = function->getStatistic();
      Residual residual = statistic.getDisplayResidual(obs->getId());

      // get deviation attributes
      std::string attrVx =
          getObservationDisplayAttributesName(eObservationDisplayVX);
      std::string attrVy =
          getObservationDisplayAttributesName(eObservationDisplayVY);
      std::string attrVz =
          getObservationDisplayAttributesName(eObservationDisplayVZ);
      std::string attrV =
          getObservationDisplayAttributesName(eObservationDisplayV);
      if (residual.elementId == obs->getId() &&
          residual.corrections.contains(attrVx) &&
          residual.corrections.contains(attrVy) &&
          residual.corrections.contains(attrVz) &&
          residual.corrections.contains(attrV)) {
        auto* deviation = observation->mutable_deviation();
        deviation->set_vx(residual.corrections[attrVx]);
        deviation->set_vy(residual.corrections[attrVy]);
        deviation->set_vz(residual.corrections[attrVz]);
        deviation->set_v(residual.corrections[attrV]);
      }

      // set up use state
      observation->set_used(function->getIsUsed(0, obs->getId()));
    }
  }
  return ErrorCode::eNoError;
}

/*!
 * \brief OiRequestHandler::removeObservations
 */
OiRequestResponse::ErrorCode OiRequestHandler::removeObservations(
    const Request& request, Response& response) {
  // check request
  const auto& message = request.remove_observations();
  if (!message.has_id() || !message.has_observations()) {
    return ErrorCode::eWrongFormat;
  }

  // get and check feature
  oi::Guarded<FeatureWrapper> feature =
      this->currentJob->getFeatureById(message.id());
  if (feature.isNull() || feature->getGeometry().isNull()) {
    return ErrorCode::eNoFeatureWithId;
  }

  // get a list of observations and functions of the feature
  std::vector<oi::Guarded<Observation> > featureObservations =
      feature->getGeometry()->getObservations();
  std::vector<oi::Guarded<Function> > featureFunctions =
      feature->getGeometry()->getFunctions();

  // get a list of observation id's that shall be removed
  std::vector<int> obs2Remove;
  for (const int id : message.observations().ids()) {
    obs2Remove.push_back(id);
  }

  // run through all observations and remove the marked ones
  foreach (const oi::Guarded<Observation>& obs, featureObservations) {
    // check observation
    if (obs.isNull() || !oi::contains(obs2Remove, obs->getId())) {
      continue;
    }

    // remove observation from functions
    foreach (const oi::Guarded<Function>& function, featureFunctions) {
      if (!function.isNull()) {
        function->removeInputElement(obs->getId());
      }
    }

    // reset dependency between feature and observation
    obs->removeTargetGeometry(feature->getGeometry()->getId());
  }

  response.mutable_remove_observations();
  return ErrorCode::eNoError;
}

/*!
 * \brief OiRequestHandler::getParameters
 */
OiRequestResponse::ErrorCode OiRequestHandler::getParameters(
    const Request& request, Response& response) {
  // check request
  const auto& message = request.get_parameters();
  if (!message.has_id()) {
    return ErrorCode::eWrongFormat;
  }

  // get and check feature by id
  oi::Guarded<FeatureWrapper> feature =
      this->currentJob->getFeatureById(message.id());
  if (feature.isNull() || feature->getFeature().isNull()) {
    return ErrorCode::eNoFeatureWithId;
  }

  // add feature information
  auto* payload = response.mutable_get_parameters();
  payload->set_id(message.id());
  payload->set_name(feature->getFeature()->getFeatureName());
  payload->set_kind(oi::codec::toProto(feature->getFeatureTypeEnum()));
  payload->set_group(feature->getFeature()->getGroupName());
  payload->set_solved(feature->getFeature()->getIsSolved());
  payload->set_nominal(
      !(feature->getGeometry().isNull() ||
        feature->getGeometry()->getOwnerDescription().has_value()));

  // add parameters
  const std::map<QString, double> values = parametersOf(feature);
  for (auto it = values.cbegin(); it != values.cend(); ++it) {
    auto* parameter = payload->add_parameters();
    parameter->set_name(it->first.toStdString());
    parameter->set_value(it->second);
  }

  // add standard deviation
  if (getIsGeometry(feature->getFeatureTypeEnum())) {
    payload->set_stdev(feature->getGeometry()->getStatistic().getStdev());
  }
  return ErrorCode::eNoError;
}

/*!
 * \brief OiRequestHandler::getMeasurementConfigs
 */
OiRequestResponse::ErrorCode OiRequestHandler::getMeasurementConfigs(
    const Request& /*request*/, Response& response) {
  // check measurement config manager
  if (this->measurementConfigManager.isNull()) {
    return ErrorCode::eNoMeasurementConfigManager;
  }

  // add configs
  auto* payload = response.mutable_get_measurement_configs();
  foreach (const MeasurementConfig& mConfig,
           this->measurementConfigManager->getConfigs()) {
    *payload->add_configs() = configEntry(mConfig, mConfig.isUserConfig());
  }
  return ErrorCode::eNoError;
}

/*!
 * \brief OiRequestHandler::getMeasurementConfig
 */
OiRequestResponse::ErrorCode OiRequestHandler::getMeasurementConfig(
    const Request& request, Response& response) {
  // check measurement config manager
  if (this->measurementConfigManager.isNull()) {
    return ErrorCode::eNoMeasurementConfigManager;
  }

  // check request
  const auto& message = request.get_measurement_config();
  if (!message.has_id()) {
    return ErrorCode::eWrongFormat;
  }

  // get and check feature by id
  oi::Guarded<FeatureWrapper> feature =
      this->currentJob->getFeatureById(message.id());
  if (feature.isNull() || feature->getGeometry().isNull()) {
    return ErrorCode::eNoFeatureWithId;
  }

  // add config
  MeasurementConfig mConfig = this->measurementConfigManager->getConfig(
      feature->getGeometry()->getMeasurementConfig());

  auto* payload = response.mutable_get_measurement_config();
  payload->set_id(message.id());
  *payload->mutable_config() = configEntry(mConfig, false);
  return ErrorCode::eNoError;
}

/*!
 * \brief OiRequestHandler::setMeasurementConfig
 */
OiRequestResponse::ErrorCode OiRequestHandler::setMeasurementConfig(
    const Request& request, Response& response) {
  // check measurement config manager
  if (this->measurementConfigManager.isNull()) {
    return ErrorCode::eNoMeasurementConfigManager;
  }

  // check request
  const auto& message = request.set_measurement_config();
  if (!message.has_id() || !message.has_config_name() || !message.has_saved()) {
    return ErrorCode::eWrongFormat;
  }

  // get and check feature by id
  oi::Guarded<FeatureWrapper> feature =
      this->currentJob->getFeatureById(message.id());
  if (feature.isNull() || feature->getGeometry().isNull()) {
    return ErrorCode::eNoFeatureWithId;
  }

  // get and check measurement config
  const QString name = toQString(message.config_name());
  MeasurementConfig mConfig;
  if (message.saved()) {
    mConfig = this->measurementConfigManager->getUserConfig(name);
  } else {
    mConfig = this->measurementConfigManager->getProjectConfig(name);
  }
  if (!mConfig.isValid()) {
    return ErrorCode::eNoMeasurementConfig;
  }

  // pass measurement config to feature
  feature->getGeometry()->setMeasurementConfig(mConfig.getKey());

  response.mutable_set_measurement_config();
  return ErrorCode::eNoError;
}

/*!
 * \brief OiRequestHandler::getCoordinateSystems
 */
OiRequestResponse::ErrorCode OiRequestHandler::getCoordinateSystems(
    const Request& /*request*/, Response& response) {
  // get all coordinate systems
  std::vector<oi::Guarded<CoordinateSystem> > systems =
      this->currentJob->getCoordinateSystemsList();
  std::vector<oi::Guarded<CoordinateSystem> > stations =
      this->currentJob->getStationSystemsList();

  // describe the coordinate systems, then the station systems
  auto* payload = response.mutable_get_coordinate_systems();
  for (const auto* list : {&systems, &stations}) {
    foreach (const oi::Guarded<CoordinateSystem>& system, *list) {
      // check coordinate system
      if (system.isNull()) {
        continue;
      }

      auto* summary = payload->add_systems();
      summary->set_id(system->getId());
      summary->set_name(system->getFeatureName());
      summary->set_group(system->getGroupName());
    }
  }
  return ErrorCode::eNoError;
}

/*!
 * \brief OiRequestHandler::parametersOf
 * The parameters of a feature, depending on its type, as name/value pairs
 * sorted by name
 * \param feature
 */
std::map<QString, double> OiRequestHandler::parametersOf(
    const oi::Guarded<FeatureWrapper>& feature) {
  std::map<QString, double> values;

  // get parameter/value pairs
  switch (feature->getFeatureTypeEnum()) {
    case FeatureTypes::eCircleFeature:
      if (feature->getCircle().isNull()) {
        return values;
      }
      values.insert_or_assign(
          "x", feature->getCircle()->getPosition().getVector().getAt(0));
      values.insert_or_assign(
          "y", feature->getCircle()->getPosition().getVector().getAt(1));
      values.insert_or_assign(
          "z", feature->getCircle()->getPosition().getVector().getAt(2));
      values.insert_or_assign(
          "i", feature->getCircle()->getDirection().getVector().getAt(0));
      values.insert_or_assign(
          "j", feature->getCircle()->getDirection().getVector().getAt(1));
      values.insert_or_assign(
          "k", feature->getCircle()->getDirection().getVector().getAt(2));
      values.insert_or_assign("radius",
                              feature->getCircle()->getRadius().getRadius());
      break;
    case FeatureTypes::eConeFeature:
      if (feature->getCone().isNull()) {
        return values;
      }
      values.insert_or_assign(
          "x", feature->getCone()->getPosition().getVector().getAt(0));
      values.insert_or_assign(
          "y", feature->getCone()->getPosition().getVector().getAt(1));
      values.insert_or_assign(
          "z", feature->getCone()->getPosition().getVector().getAt(2));
      values.insert_or_assign(
          "i", feature->getCone()->getDirection().getVector().getAt(0));
      values.insert_or_assign(
          "j", feature->getCone()->getDirection().getVector().getAt(1));
      values.insert_or_assign(
          "k", feature->getCone()->getDirection().getVector().getAt(2));
      values.insert_or_assign("aperture", feature->getCone()->getAperture());
      break;
    case FeatureTypes::eCylinderFeature:
      if (feature->getCylinder().isNull()) {
        return values;
      }
      values.insert_or_assign(
          "x", feature->getCylinder()->getPosition().getVector().getAt(0));
      values.insert_or_assign(
          "y", feature->getCylinder()->getPosition().getVector().getAt(1));
      values.insert_or_assign(
          "z", feature->getCylinder()->getPosition().getVector().getAt(2));
      values.insert_or_assign(
          "i", feature->getCylinder()->getDirection().getVector().getAt(0));
      values.insert_or_assign(
          "j", feature->getCylinder()->getDirection().getVector().getAt(1));
      values.insert_or_assign(
          "k", feature->getCylinder()->getDirection().getVector().getAt(2));
      values.insert_or_assign("radius",
                              feature->getCylinder()->getRadius().getRadius());
      break;
    case FeatureTypes::eEllipseFeature:
      if (feature->getEllipse().isNull()) {
        return values;
      }
      values.insert_or_assign(
          "x", feature->getEllipse()->getPosition().getVector().getAt(0));
      values.insert_or_assign(
          "y", feature->getEllipse()->getPosition().getVector().getAt(1));
      values.insert_or_assign(
          "z", feature->getEllipse()->getPosition().getVector().getAt(2));
      values.insert_or_assign(
          "i", feature->getEllipse()->getDirection().getVector().getAt(0));
      values.insert_or_assign(
          "j", feature->getEllipse()->getDirection().getVector().getAt(1));
      values.insert_or_assign(
          "k", feature->getEllipse()->getDirection().getVector().getAt(2));
      values.insert_or_assign("a", feature->getEllipse()->getA());
      values.insert_or_assign("b", feature->getEllipse()->getB());
      values.insert_or_assign(
          "i 2",
          feature->getEllipse()->getSemiMajorAxisDirection().getVector().getAt(
              0));
      values.insert_or_assign(
          "j 2",
          feature->getEllipse()->getSemiMajorAxisDirection().getVector().getAt(
              1));
      values.insert_or_assign(
          "k 2",
          feature->getEllipse()->getSemiMajorAxisDirection().getVector().getAt(
              2));
      break;
    case FeatureTypes::eEllipsoidFeature:
      if (feature->getEllipsoid().isNull()) {
        return values;
      }
      values.insert_or_assign(
          "x", feature->getEllipsoid()->getPosition().getVector().getAt(0));
      values.insert_or_assign(
          "y", feature->getEllipsoid()->getPosition().getVector().getAt(1));
      values.insert_or_assign(
          "z", feature->getEllipsoid()->getPosition().getVector().getAt(2));
      values.insert_or_assign(
          "i", feature->getEllipsoid()->getDirection().getVector().getAt(0));
      values.insert_or_assign(
          "j", feature->getEllipsoid()->getDirection().getVector().getAt(1));
      values.insert_or_assign(
          "k", feature->getEllipsoid()->getDirection().getVector().getAt(2));
      values.insert_or_assign("a", feature->getEllipsoid()->getA());
      values.insert_or_assign("b", feature->getEllipsoid()->getB());
      break;
    case FeatureTypes::eHyperboloidFeature:
      if (feature->getHyperboloid().isNull()) {
        return values;
      }
      values.insert_or_assign(
          "x", feature->getHyperboloid()->getPosition().getVector().getAt(0));
      values.insert_or_assign(
          "y", feature->getHyperboloid()->getPosition().getVector().getAt(1));
      values.insert_or_assign(
          "z", feature->getHyperboloid()->getPosition().getVector().getAt(2));
      values.insert_or_assign(
          "i", feature->getHyperboloid()->getDirection().getVector().getAt(0));
      values.insert_or_assign(
          "j", feature->getHyperboloid()->getDirection().getVector().getAt(1));
      values.insert_or_assign(
          "k", feature->getHyperboloid()->getDirection().getVector().getAt(2));
      values.insert_or_assign("a", feature->getHyperboloid()->getA());
      values.insert_or_assign("c", feature->getHyperboloid()->getC());
      break;
    case FeatureTypes::eLineFeature:
      if (feature->getLine().isNull()) {
        return values;
      }
      values.insert_or_assign(
          "x", feature->getLine()->getPosition().getVector().getAt(0));
      values.insert_or_assign(
          "y", feature->getLine()->getPosition().getVector().getAt(1));
      values.insert_or_assign(
          "z", feature->getLine()->getPosition().getVector().getAt(2));
      values.insert_or_assign(
          "i", feature->getLine()->getDirection().getVector().getAt(0));
      values.insert_or_assign(
          "j", feature->getLine()->getDirection().getVector().getAt(1));
      values.insert_or_assign(
          "k", feature->getLine()->getDirection().getVector().getAt(2));
      break;
    case FeatureTypes::eNurbsFeature:
      break;
    case FeatureTypes::eParaboloidFeature:
      if (feature->getParaboloid().isNull()) {
        return values;
      }
      values.insert_or_assign(
          "x", feature->getParaboloid()->getPosition().getVector().getAt(0));
      values.insert_or_assign(
          "y", feature->getParaboloid()->getPosition().getVector().getAt(1));
      values.insert_or_assign(
          "z", feature->getParaboloid()->getPosition().getVector().getAt(2));
      values.insert_or_assign(
          "i", feature->getParaboloid()->getDirection().getVector().getAt(0));
      values.insert_or_assign(
          "j", feature->getParaboloid()->getDirection().getVector().getAt(1));
      values.insert_or_assign(
          "k", feature->getParaboloid()->getDirection().getVector().getAt(2));
      values.insert_or_assign("a", feature->getParaboloid()->getA());
      break;
    case FeatureTypes::ePlaneFeature:
      if (feature->getPlane().isNull()) {
        return values;
      }
      values.insert_or_assign(
          "x", feature->getPlane()->getPosition().getVector().getAt(0));
      values.insert_or_assign(
          "y", feature->getPlane()->getPosition().getVector().getAt(1));
      values.insert_or_assign(
          "z", feature->getPlane()->getPosition().getVector().getAt(2));
      values.insert_or_assign(
          "i", feature->getPlane()->getDirection().getVector().getAt(0));
      values.insert_or_assign(
          "j", feature->getPlane()->getDirection().getVector().getAt(1));
      values.insert_or_assign(
          "k", feature->getPlane()->getDirection().getVector().getAt(2));
      break;
    case FeatureTypes::ePointFeature:
      if (feature->getPoint().isNull()) {
        return values;
      }
      values.insert_or_assign(
          "x", feature->getPoint()->getPosition().getVector().getAt(0));
      values.insert_or_assign(
          "y", feature->getPoint()->getPosition().getVector().getAt(1));
      values.insert_or_assign(
          "z", feature->getPoint()->getPosition().getVector().getAt(2));
    case FeatureTypes::ePointCloudFeature:
      break;
    case FeatureTypes::eScalarEntityAngleFeature:
      if (feature->getScalarEntityAngle().isNull()) {
        return values;
      }
      values.insert_or_assign("angle",
                              feature->getScalarEntityAngle()->getAngle());
      break;
    case FeatureTypes::eScalarEntityDistanceFeature:
      if (feature->getScalarEntityDistance().isNull()) {
        return values;
      }
      values.insert_or_assign(
          "distance", feature->getScalarEntityDistance()->getDistance());
      break;
    case FeatureTypes::eScalarEntityMeasurementSeriesFeature:
      if (feature->getScalarEntityMeasurementSeries().isNull()) {
        return values;
      }
      values.insert_or_assign(
          "measurement series",
          feature->getScalarEntityMeasurementSeries()->getSeriesValue());
      break;
    case FeatureTypes::eScalarEntityTemperatureFeature:
      if (feature->getScalarEntityTemperature().isNull()) {
        return values;
      }
      values.insert_or_assign(
          "temperature",
          feature->getScalarEntityTemperature()->getTemperature());
      break;
    case FeatureTypes::eSlottedHoleFeature:
      if (feature->getSlottedHole().isNull()) {
        return values;
      }
      values.insert_or_assign(
          "x", feature->getSlottedHole()->getPosition().getVector().getAt(0));
      values.insert_or_assign(
          "y", feature->getSlottedHole()->getPosition().getVector().getAt(1));
      values.insert_or_assign(
          "z", feature->getSlottedHole()->getPosition().getVector().getAt(2));
      values.insert_or_assign(
          "i", feature->getSlottedHole()->getDirection().getVector().getAt(0));
      values.insert_or_assign(
          "j", feature->getSlottedHole()->getDirection().getVector().getAt(1));
      values.insert_or_assign(
          "k", feature->getSlottedHole()->getDirection().getVector().getAt(2));
      values.insert_or_assign(
          "radius", feature->getSlottedHole()->getRadius().getRadius());
      values.insert_or_assign("length", feature->getSlottedHole()->getLength());
      values.insert_or_assign(
          "i", feature->getSlottedHole()->getHoleAxis().getVector().getAt(0));
      values.insert_or_assign(
          "j", feature->getSlottedHole()->getHoleAxis().getVector().getAt(1));
      values.insert_or_assign(
          "k", feature->getSlottedHole()->getHoleAxis().getVector().getAt(2));
      break;
    case FeatureTypes::eSphereFeature:
      if (feature->getSphere().isNull()) {
        return values;
      }
      values.insert_or_assign(
          "x", feature->getSphere()->getPosition().getVector().getAt(0));
      values.insert_or_assign(
          "y", feature->getSphere()->getPosition().getVector().getAt(1));
      values.insert_or_assign(
          "z", feature->getSphere()->getPosition().getVector().getAt(2));
      values.insert_or_assign("radius",
                              feature->getSphere()->getRadius().getRadius());
      break;
    case FeatureTypes::eTorusFeature:
      if (feature->getTorus().isNull()) {
        return values;
      }
      values.insert_or_assign(
          "x", feature->getTorus()->getPosition().getVector().getAt(0));
      values.insert_or_assign(
          "y", feature->getTorus()->getPosition().getVector().getAt(1));
      values.insert_or_assign(
          "z", feature->getTorus()->getPosition().getVector().getAt(2));
      values.insert_or_assign(
          "i", feature->getTorus()->getDirection().getVector().getAt(0));
      values.insert_or_assign(
          "j", feature->getTorus()->getDirection().getVector().getAt(1));
      values.insert_or_assign(
          "k", feature->getTorus()->getDirection().getVector().getAt(2));
      values.insert_or_assign("radius",
                              feature->getTorus()->getRadius().getRadius());
      values.insert_or_assign(
          "radius 2", feature->getTorus()->getSmallRadius().getRadius());
      break;
    case FeatureTypes::eCoordinateSystemFeature:
      if (feature->getCoordinateSystem().isNull()) {
        return values;
      }
      values.insert_or_assign(
          "x",
          feature->getCoordinateSystem()->getOrigin().getVector().getAt(0));
      values.insert_or_assign(
          "y",
          feature->getCoordinateSystem()->getOrigin().getVector().getAt(1));
      values.insert_or_assign(
          "z",
          feature->getCoordinateSystem()->getOrigin().getVector().getAt(2));
      values.insert_or_assign(
          "i", feature->getCoordinateSystem()->getXAxis().getVector().getAt(0));
      values.insert_or_assign(
          "j", feature->getCoordinateSystem()->getXAxis().getVector().getAt(1));
      values.insert_or_assign(
          "k", feature->getCoordinateSystem()->getXAxis().getVector().getAt(2));
      values.insert_or_assign(
          "i 2",
          feature->getCoordinateSystem()->getYAxis().getVector().getAt(0));
      values.insert_or_assign(
          "j 2",
          feature->getCoordinateSystem()->getYAxis().getVector().getAt(1));
      values.insert_or_assign(
          "k 2",
          feature->getCoordinateSystem()->getYAxis().getVector().getAt(2));
      values.insert_or_assign(
          "i 3",
          feature->getCoordinateSystem()->getZAxis().getVector().getAt(0));
      values.insert_or_assign(
          "j 3",
          feature->getCoordinateSystem()->getZAxis().getVector().getAt(1));
      values.insert_or_assign(
          "k 3",
          feature->getCoordinateSystem()->getZAxis().getVector().getAt(2));
      break;
    case FeatureTypes::eStationFeature:
      if (feature->getStation().isNull()) {
        return values;
      }
      values.insert_or_assign(
          "x",
          feature->getStation()->getPosition()->getPosition().getVector().getAt(
              0));
      values.insert_or_assign(
          "y",
          feature->getStation()->getPosition()->getPosition().getVector().getAt(
              1));
      values.insert_or_assign(
          "z",
          feature->getStation()->getPosition()->getPosition().getVector().getAt(
              2));
      break;
    case FeatureTypes::eTrafoParamFeature:
      if (feature->getTrafoParam().isNull()) {
        return values;
      }
      values.insert_or_assign(
          "tx", feature->getTrafoParam()->getTranslation().getAt(0));
      values.insert_or_assign(
          "ty", feature->getTrafoParam()->getTranslation().getAt(1));
      values.insert_or_assign(
          "tz", feature->getTrafoParam()->getTranslation().getAt(2));
      values.insert_or_assign("rx",
                              feature->getTrafoParam()->getRotation().getAt(0));
      values.insert_or_assign("ry",
                              feature->getTrafoParam()->getRotation().getAt(1));
      values.insert_or_assign("rz",
                              feature->getTrafoParam()->getRotation().getAt(2));
      values.insert_or_assign("sx",
                              feature->getTrafoParam()->getScale().getAt(0));
      values.insert_or_assign("sy",
                              feature->getTrafoParam()->getScale().getAt(1));
      values.insert_or_assign("sz",
                              feature->getTrafoParam()->getScale().getAt(2));
      break;
  }
  return values;
}

/*!
 * \brief OiRequestHandler::sendErrorMessage
 * Answers with an error and no payload
 * \param request
 * \param requestId the id of the request, echoed to the client
 * \param type
 * \param error
 */
void OiRequestHandler::sendErrorMessage(OiRequestResponse request,
                                        int requestId,
                                        OiRequestResponse::RequestType type,
                                        OiRequestResponse::ErrorCode error) {
  oipb::Envelope envelope;
  envelope.set_request_id(requestId);
  auto* response = envelope.mutable_response();
  response->set_request(oi::codec::toProto(type));
  response->set_error(oi::codec::toProto(error));

  request.myRequestType = type;
  request.errorCode = error;
  request.response = serialize(envelope);
  emit this->sendResponse(request);
}

/*!
 * \brief OiRequestHandler::buildWatchWindowMessage
 * \param reading: the notification payload that shall be filled
 * \param streamData: the sensor stream data
 * \return
 */
bool OiRequestHandler::buildWatchWindowMessage(
    openindy::v1::RealTimeReading& reading, const QVariantMap& streamData) {
  // check current job
  if (this->currentJob.isNull()) {
    return false;
  }

  // get and check active feature
  oi::Guarded<FeatureWrapper> activeFeature =
      this->currentJob->getActiveFeature();
  if (activeFeature.isNull() || activeFeature->getGeometry().isNull() ||
      !activeFeature->getGeometry()->getIsSolved()) {
    return false;
  }

  // get and check active coordinate system
  oi::Guarded<CoordinateSystem> activeSystem =
      this->currentJob->getActiveCoordinateSystem();
  if (activeSystem.isNull()) {
    return false;
  }

  // get and check station
  oi::Guarded<Station> activeStation = this->currentJob->getActiveStation();
  if (activeStation.isNull() || activeStation->getCoordinateSystem().isNull()) {
    return false;
  }

  // get xyz of active geometry
  OiVec xyz = activeFeature->getGeometry()->getPosition().getVectorH();

  // get transformation matrix (station -> active system)
  TrafoController trafoControl;
  OiMat tMat(4, 4);
  if (!trafoControl.getTransformationMatrix(
          tMat, activeStation->getCoordinateSystem(), activeSystem)) {
    return false;
  }

  // get and transform sensor stream data
  switch (this->watchWindowTask.type) {
    case ReadingTypes::eCartesianReading: {
      // check if x, y, z are available
      if (!streamData.contains("x") || !streamData.contains("y") ||
          !streamData.contains("z")) {
        return false;
      }

      // get sensor xyz
      OiVec streamXyz(4);
      streamXyz.setAt(0, streamData.value("x").toDouble());
      streamXyz.setAt(1, streamData.value("y").toDouble());
      streamXyz.setAt(2, streamData.value("z").toDouble());
      streamXyz.setAt(3, 1.0);

      // transform sensor xyz
      streamXyz = tMat * streamXyz;

      // build difference sensor - feature
      OiVec diff = streamXyz - xyz;

      // fill the reading
      auto* offset = reading.mutable_cartesian();
      offset->set_x(diff.getAt(0));
      offset->set_y(diff.getAt(1));
      offset->set_z(diff.getAt(2));

      return true;
    }
    case ReadingTypes::ePolarReading: {
      // check if azimuth, zenith, distance are available
      if (!streamData.contains("azimuth") || !streamData.contains("zenith") ||
          !streamData.contains("distance")) {
        return false;
      }

      // get sensor xyz
      double azimuth = NAN, zenith = NAN, distance = NAN;
      azimuth = streamData.value("azimuth").toDouble();
      zenith = streamData.value("zenith").toDouble();
      distance = streamData.value("distance").toDouble();
      OiVec streamXyz(3);
      streamXyz = Reading::toCartesian(azimuth, zenith, distance);
      streamXyz.add(1.0);

      // transform sensor xyz
      streamXyz = tMat * streamXyz;

      // transform sensor cartesian to polar
      double x = NAN, y = NAN, z = NAN;
      x = streamXyz.getAt(0);
      y = streamXyz.getAt(1);
      z = streamXyz.getAt(2);
      OiVec streamPolar(3);
      streamPolar = Reading::toPolar(x, y, z);

      // transform feature cartesian to polar
      x = xyz.getAt(0);
      y = xyz.getAt(1);
      z = xyz.getAt(2);
      OiVec featurePolar(3);
      featurePolar = Reading::toPolar(x, y, z);

      // build difference sensor - feature
      OiVec diff = streamPolar - featurePolar;

      // fill the reading
      auto* offset = reading.mutable_polar();
      offset->set_azimuth(diff.getAt(0));
      offset->set_zenith(diff.getAt(1));
      offset->set_distance(diff.getAt(2));

      return true;
    }
    default:
      return false;
  }

  return false;
}

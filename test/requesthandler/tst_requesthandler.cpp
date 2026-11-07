#include <QByteArray>
#include <QPointer>
#include <QtTest>
#include <string>
#include <vector>

#include "chooselalib.h"
#include "coordinatesystem.h"
#include "featureutil.h"
#include "featurewrapper.h"
#include "geometry/plane.h"
#include "geometry/point.h"
#include "measurementconfigmanager.h"
#include "network_convert.h"
#include "oijob.h"
#include "oirequesthandler.h"
#include "openindy/network.pb.h"
#include "openindy/project.pb.h"
#include "qtconvert.h"
#include "station.h"

using namespace oi;
using namespace oi::math;
namespace oipb = openindy::v1;

/*!
 * The websocket remote-control protocol, driven through OiRequestHandler the
 * way OiWebSocket does it: a serialized Envelope goes in as the carrier's
 * request bytes, the answer comes out of the sendResponse signal as
 * serialized bytes, and is decoded here.
 */
namespace {

// a project needs an active station and coordinate system; same minimal setup
// as test/projectexchange
oi::Guarded<OiJob> makeJobWithStationAndSystem() {
  oi::Guarded<OiJob> job = new OiJob();

  oi::Guarded<FeatureWrapper> stationFeature = new FeatureWrapper();
  oi::Guarded<Station> station = new Station();
  station->setFeatureName("STATION01");
  stationFeature->setStation(station);
  job->addFeature(stationFeature);

  oi::Guarded<FeatureWrapper> systemFeature = new FeatureWrapper();
  oi::Guarded<CoordinateSystem> system = new CoordinateSystem();
  system->setFeatureName("PART");
  systemFeature->setCoordinateSystem(system);
  job->addFeature(systemFeature);

  station->setActiveStationState(true);
  station->getCoordinateSystem()->setActiveCoordinateSystemState(true);

  FeatureAttributes bundleAttr;
  bundleAttr.count = 1;
  bundleAttr.typeOfFeature = FeatureTypes::eCoordinateSystemFeature;
  bundleAttr.name = "Bundle01";
  bundleAttr.isBundleSystem = true;
  job->addFeatures(bundleAttr);
  FeatureUtil().createInitialBundleTransformation(job);

  return job;
}

oi::Guarded<FeatureWrapper> addPoint(const oi::Guarded<OiJob>& job,
                                     const QString& name, double x, double y,
                                     double z) {
  oi::Guarded<FeatureWrapper> wrapper = new FeatureWrapper();
  oi::Guarded<Point> point = new Point(Position(x, y, z));
  point->setFeatureName(toStd(name));
  point->setIsSolved(true);
  wrapper->setPoint(point);
  job->addFeature(wrapper);
  return wrapper;
}

QByteArray bytesOf(const oipb::Envelope& envelope) {
  const std::string bytes = envelope.SerializeAsString();
  return QByteArray(bytes.data(), static_cast<qsizetype>(bytes.size()));
}

// what the handler sent for one request
struct Answer {
  bool accepted{};
  int sent{};  // how many carriers came out of sendResponse
  OiRequestResponse carrier;
  oipb::Envelope envelope;

  const oipb::Response& response() const { return envelope.response(); }
};

}  // namespace

class RequestHandlerTest : public QObject {
  Q_OBJECT

 private slots:
  void initTestCase() {
    ChooseLALib::setLinearAlgebra(ChooseLALib::LinearAlgebraLibrary::Armadillo);
  }
  void init();

  void getProject();
  void activeFeature();
  void activeStation();
  void activeCoordinateSystem();
  void getFeatures();
  void getCoordinateSystems();
  void getParameters();
  void getObservations();
  void measurementConfigs();
  void setAndGetMeasurementConfig();
  void addFeatures();
  void aimNeedsASensor();
  void watchwindowChecks();
  void noJob();
  void garbageBytes();
  void notAnEnvelopeOfRequest();
  void missingPayload();
  void unknownRequest();
  void toolRequestIsNotAnswered();
  void notifications();
  void realTimeReadingWithoutWatchwindow();

 private:
  Answer send(const oipb::Envelope& envelope, int requesterId = 7) {
    return sendBytes(bytesOf(envelope), requesterId);
  }

  Answer sendBytes(const QByteArray& bytes, int requesterId = 7) {
    Answer answer;
    std::vector<OiRequestResponse> sent;
    const auto connection = QObject::connect(
        &handler, &OiRequestHandler::sendResponse, this,
        [&sent](OiRequestResponse r) { sent.push_back(r); },
        Qt::DirectConnection);
    OiRequestResponse request;
    request.requesterId = requesterId;
    request.request = toStdBytes(bytes);
    answer.accepted = handler.receiveRequest(request);
    QObject::disconnect(connection);
    answer.sent = static_cast<int>(sent.size());
    if (!sent.empty()) {
      answer.carrier = sent.back();
      answer.envelope.ParseFromArray(
          answer.carrier.response.data(),
          static_cast<int>(answer.carrier.response.size()));
    }
    return answer;
  }

  // run one request given a function that fills the Request
  template <class Fill>
  Answer ask(Fill fill, int requestId = 11) {
    oipb::Envelope envelope;
    envelope.set_request_id(requestId);
    fill(*envelope.mutable_request());
    return send(envelope, 7);
  }

  static void checkAnswered(const Answer& answer, oipb::RequestKind kind,
                            oipb::ErrorCode error, int requestId = 11) {
    QCOMPARE(answer.sent, 1);
    QCOMPARE(answer.envelope.content_case(), oipb::Envelope::kResponse);
    QCOMPARE(answer.envelope.request_id(), requestId);
    QCOMPARE(static_cast<int>(answer.response().request()),
             static_cast<int>(kind));
    QCOMPARE(static_cast<int>(answer.response().error()),
             static_cast<int>(error));
    QCOMPARE(static_cast<int>(*oi::codec::fromProto(error)),
             static_cast<int>(answer.carrier.errorCode));
    QCOMPARE(answer.carrier.requesterId, 7);
  }

  oi::Guarded<OiJob> job;
  oi::Guarded<FeatureWrapper> point;
  QPointer<MeasurementConfigManager> configs;
  OiRequestHandler handler;
};

void RequestHandlerTest::init() {
  job = makeJobWithStationAndSystem();
  point = addPoint(job, "P1", 1.0, 2.0, 3.0);
  delete configs;
  configs = new MeasurementConfigManager();
  MeasurementConfig config;
  config.setName("ProjectPoint");
  config.setMeasurementMode(MeasurementModes::ePrecise_MeasurementMode);
  config.setMeasurementType(MeasurementTypes::eSinglePoint_MeasurementType);
  config.setMaxObservations(9);
  config.makeProjectConfig();
  configs->addProjectConfig(config);

  handler.setCurrentJob(job);
  handler.setMeasurementConfigManager(configs);
}

void RequestHandlerTest::getProject() {
  const auto answer =
      ask([](oipb::Request& r) { r.mutable_get_project(); }, 21);
  checkAnswered(answer, oipb::REQUEST_KIND_GET_PROJECT, oipb::ERROR_CODE_NONE,
                21);
  QVERIFY(answer.accepted);
  QCOMPARE(answer.response().payload_case(), oipb::Response::kGetProject);
  const std::string& bytes = answer.response().get_project().project();
  QVERIFY(!bytes.empty());
  oipb::Project project;
  QVERIFY(project.ParseFromString(bytes));
}

void RequestHandlerTest::activeFeature() {
  // none yet
  auto answer = ask([](oipb::Request& r) { r.mutable_get_active_feature(); });
  checkAnswered(answer, oipb::REQUEST_KIND_GET_ACTIVE_FEATURE,
                oipb::ERROR_CODE_NO_ACTIVE_FEATURE);
  QCOMPARE(answer.response().get_active_feature().id(), 0);
  QVERIFY(answer.accepted);

  // set it
  const int id = point->getFeature()->getId();
  answer = ask(
      [id](oipb::Request& r) { r.mutable_set_active_feature()->set_id(id); });
  checkAnswered(answer, oipb::REQUEST_KIND_SET_ACTIVE_FEATURE,
                oipb::ERROR_CODE_NONE);
  QCOMPARE(answer.response().set_active_feature().id(), id);
  QVERIFY(job->getActiveFeature()->getFeature() == point->getFeature());

  // and read it back
  answer = ask([](oipb::Request& r) { r.mutable_get_active_feature(); });
  checkAnswered(answer, oipb::REQUEST_KIND_GET_ACTIVE_FEATURE,
                oipb::ERROR_CODE_NONE);
  QCOMPARE(answer.response().get_active_feature().id(), id);

  // an id nobody has: error, the active feature stays
  answer = ask(
      [](oipb::Request& r) { r.mutable_set_active_feature()->set_id(987654); });
  checkAnswered(answer, oipb::REQUEST_KIND_SET_ACTIVE_FEATURE,
                oipb::ERROR_CODE_NO_FEATURE_WITH_ID);
  QCOMPARE(answer.response().set_active_feature().id(), id);

  // no id at all
  answer = ask([](oipb::Request& r) { r.mutable_set_active_feature(); });
  checkAnswered(answer, oipb::REQUEST_KIND_SET_ACTIVE_FEATURE,
                oipb::ERROR_CODE_WRONG_FORMAT);
  QCOMPARE(answer.response().set_active_feature().id(), id);
}

void RequestHandlerTest::activeStation() {
  const int stationId = job->getActiveStation()->getId();

  auto answer = ask([](oipb::Request& r) { r.mutable_get_active_station(); });
  checkAnswered(answer, oipb::REQUEST_KIND_GET_ACTIVE_STATION,
                oipb::ERROR_CODE_NONE);
  QCOMPARE(answer.response().get_active_station().id(), stationId);

  answer = ask([stationId](oipb::Request& r) {
    r.mutable_set_active_station()->set_id(stationId);
  });
  checkAnswered(answer, oipb::REQUEST_KIND_SET_ACTIVE_STATION,
                oipb::ERROR_CODE_NONE);
  QCOMPARE(answer.response().set_active_station().id(), stationId);

  // a point is no station
  const int pointId = point->getFeature()->getId();
  answer = ask([pointId](oipb::Request& r) {
    r.mutable_set_active_station()->set_id(pointId);
  });
  checkAnswered(answer, oipb::REQUEST_KIND_SET_ACTIVE_STATION,
                oipb::ERROR_CODE_NO_FEATURE_WITH_ID);
  QCOMPARE(answer.response().set_active_station().id(), stationId);

  answer = ask([](oipb::Request& r) { r.mutable_set_active_station(); });
  checkAnswered(answer, oipb::REQUEST_KIND_SET_ACTIVE_STATION,
                oipb::ERROR_CODE_WRONG_FORMAT);
}

void RequestHandlerTest::activeCoordinateSystem() {
  const int systemId = job->getActiveCoordinateSystem()->getId();

  auto answer =
      ask([](oipb::Request& r) { r.mutable_get_active_coordinate_system(); });
  checkAnswered(answer, oipb::REQUEST_KIND_GET_ACTIVE_COORDINATE_SYSTEM,
                oipb::ERROR_CODE_NONE);
  QCOMPARE(answer.response().get_active_coordinate_system().id(), systemId);

  answer = ask([systemId](oipb::Request& r) {
    r.mutable_set_active_coordinate_system()->set_id(systemId);
  });
  checkAnswered(answer, oipb::REQUEST_KIND_SET_ACTIVE_COORDINATE_SYSTEM,
                oipb::ERROR_CODE_NONE);
  QCOMPARE(answer.response().set_active_coordinate_system().id(), systemId);

  const int pointId = point->getFeature()->getId();
  answer = ask([pointId](oipb::Request& r) {
    r.mutable_set_active_coordinate_system()->set_id(pointId);
  });
  checkAnswered(answer, oipb::REQUEST_KIND_SET_ACTIVE_COORDINATE_SYSTEM,
                oipb::ERROR_CODE_NO_FEATURE_WITH_ID);

  answer =
      ask([](oipb::Request& r) { r.mutable_set_active_coordinate_system(); });
  checkAnswered(answer, oipb::REQUEST_KIND_SET_ACTIVE_COORDINATE_SYSTEM,
                oipb::ERROR_CODE_WRONG_FORMAT);
}

void RequestHandlerTest::getFeatures() {
  const auto answer = ask([](oipb::Request& r) { r.mutable_get_features(); });
  checkAnswered(answer, oipb::REQUEST_KIND_GET_FEATURES, oipb::ERROR_CODE_NONE);
  const auto& features = answer.response().get_features().features();
  QVERIFY(features.size() >= 3);

  bool found = false;
  for (const auto& feature : features) {
    QVERIFY(feature.id() != 0);
    QVERIFY(feature.kind() != oipb::FEATURE_KIND_UNSPECIFIED);
    if (feature.id() == point->getFeature()->getId()) {
      found = true;
      QCOMPARE(static_cast<int>(feature.kind()),
               static_cast<int>(oipb::FEATURE_KIND_POINT));
      QCOMPARE(QString::fromStdString(feature.name()), QString("P1"));
      QVERIFY(feature.solved());
    }
  }
  QVERIFY(found);
}

void RequestHandlerTest::getCoordinateSystems() {
  const auto answer =
      ask([](oipb::Request& r) { r.mutable_get_coordinate_systems(); });
  checkAnswered(answer, oipb::REQUEST_KIND_GET_COORDINATE_SYSTEMS,
                oipb::ERROR_CODE_NONE);
  bool part = false;
  for (const auto& system :
       answer.response().get_coordinate_systems().systems()) {
    QVERIFY(system.id() != 0);
    part = part || system.name() == "PART";
  }
  QVERIFY(part);
}

void RequestHandlerTest::getParameters() {
  const int id = point->getFeature()->getId();
  auto answer =
      ask([id](oipb::Request& r) { r.mutable_get_parameters()->set_id(id); });
  checkAnswered(answer, oipb::REQUEST_KIND_GET_PARAMETERS,
                oipb::ERROR_CODE_NONE);
  const auto& payload = answer.response().get_parameters();
  QCOMPARE(payload.id(), id);
  QCOMPARE(QString::fromStdString(payload.name()), QString("P1"));
  QCOMPARE(static_cast<int>(payload.kind()),
           static_cast<int>(oipb::FEATURE_KIND_POINT));
  QVERIFY(payload.solved());
  QVERIFY(payload.has_stdev());
  QCOMPARE(payload.parameters_size(), 3);
  QCOMPARE(QString::fromStdString(payload.parameters(0).name()), QString("x"));
  QCOMPARE(payload.parameters(0).value(), 1.0);
  QCOMPARE(payload.parameters(1).value(), 2.0);
  QCOMPARE(payload.parameters(2).value(), 3.0);

  answer =
      ask([](oipb::Request& r) { r.mutable_get_parameters()->set_id(987654); });
  checkAnswered(answer, oipb::REQUEST_KIND_GET_PARAMETERS,
                oipb::ERROR_CODE_NO_FEATURE_WITH_ID);
  QCOMPARE(answer.response().payload_case(), oipb::Response::PAYLOAD_NOT_SET);

  answer = ask([](oipb::Request& r) { r.mutable_get_parameters(); });
  checkAnswered(answer, oipb::REQUEST_KIND_GET_PARAMETERS,
                oipb::ERROR_CODE_WRONG_FORMAT);
}

void RequestHandlerTest::getObservations() {
  const int id = point->getFeature()->getId();
  auto answer =
      ask([id](oipb::Request& r) { r.mutable_get_observations()->set_id(id); });
  checkAnswered(answer, oipb::REQUEST_KIND_GET_OBSERVATIONS,
                oipb::ERROR_CODE_NONE);
  QCOMPARE(answer.response().get_observations().id(), id);
  QCOMPARE(answer.response().get_observations().observations_size(), 0);

  answer = ask([](oipb::Request& r) { r.mutable_get_observations(); });
  checkAnswered(answer, oipb::REQUEST_KIND_GET_OBSERVATIONS,
                oipb::ERROR_CODE_WRONG_FORMAT);

  answer = ask(
      [](oipb::Request& r) { r.mutable_get_observations()->set_id(987654); });
  checkAnswered(answer, oipb::REQUEST_KIND_GET_OBSERVATIONS,
                oipb::ERROR_CODE_NO_FEATURE_WITH_ID);

  // removing needs both the geometry and a selection
  answer = ask(
      [id](oipb::Request& r) { r.mutable_remove_observations()->set_id(id); });
  checkAnswered(answer, oipb::REQUEST_KIND_REMOVE_OBSERVATIONS,
                oipb::ERROR_CODE_WRONG_FORMAT);
  answer = ask([id](oipb::Request& r) {
    auto* remove = r.mutable_remove_observations();
    remove->set_id(id);
    remove->mutable_observations()->add_ids(5);
  });
  checkAnswered(answer, oipb::REQUEST_KIND_REMOVE_OBSERVATIONS,
                oipb::ERROR_CODE_NONE);
}

void RequestHandlerTest::measurementConfigs() {
  auto answer =
      ask([](oipb::Request& r) { r.mutable_get_measurement_configs(); });
  checkAnswered(answer, oipb::REQUEST_KIND_GET_MEASUREMENT_CONFIGS,
                oipb::ERROR_CODE_NONE);
  bool found = false;
  for (const auto& entry :
       answer.response().get_measurement_configs().configs()) {
    if (entry.config().name() == "ProjectPoint") {
      found = true;
      QVERIFY(!entry.saved());
      QCOMPARE(static_cast<int>(entry.config().mode()),
               static_cast<int>(oipb::MEASUREMENT_MODE_PRECISE));
      QCOMPARE(static_cast<int>(entry.config().type()),
               static_cast<int>(oipb::MEASUREMENT_TYPE_SINGLE_POINT));
      QCOMPARE(entry.config().max_observations(), 9);
    }
  }
  QVERIFY(found);

  // without a manager
  OiRequestHandler bare;
  bare.setCurrentJob(job);
  oipb::Envelope envelope;
  envelope.mutable_request()->mutable_get_measurement_configs();
  std::vector<OiRequestResponse> sent;
  QObject::connect(&bare, &OiRequestHandler::sendResponse, this,
                   [&sent](OiRequestResponse r) { sent.push_back(r); });
  OiRequestResponse request;
  request.request = toStdBytes(bytesOf(envelope));
  bare.receiveRequest(request);
  QCOMPARE(sent.size(), 1);
  QCOMPARE(static_cast<int>(sent.front().errorCode),
           static_cast<int>(
               OiRequestResponse::ErrorCode::eNoMeasurementConfigManager));
  oipb::Envelope reply;
  QVERIFY(reply.ParseFromArray(sent.front().response.data(),
                               static_cast<int>(sent.front().response.size())));
  QCOMPARE(static_cast<int>(reply.response().error()),
           static_cast<int>(oipb::ERROR_CODE_NO_MEASUREMENT_CONFIG_MANAGER));
}

void RequestHandlerTest::setAndGetMeasurementConfig() {
  const int id = point->getFeature()->getId();

  // unknown config name
  auto answer = ask([id](oipb::Request& r) {
    auto* set = r.mutable_set_measurement_config();
    set->set_id(id);
    set->set_config_name("Nope");
    set->set_saved(false);
  });
  checkAnswered(answer, oipb::REQUEST_KIND_SET_MEASUREMENT_CONFIG,
                oipb::ERROR_CODE_NO_MEASUREMENT_CONFIG);

  // a project config looked up as a saved one does not exist either
  answer = ask([id](oipb::Request& r) {
    auto* set = r.mutable_set_measurement_config();
    set->set_id(id);
    set->set_config_name("ProjectPoint");
    set->set_saved(true);
  });
  checkAnswered(answer, oipb::REQUEST_KIND_SET_MEASUREMENT_CONFIG,
                oipb::ERROR_CODE_NO_MEASUREMENT_CONFIG);

  // incomplete
  answer = ask([id](oipb::Request& r) {
    r.mutable_set_measurement_config()->set_id(id);
  });
  checkAnswered(answer, oipb::REQUEST_KIND_SET_MEASUREMENT_CONFIG,
                oipb::ERROR_CODE_WRONG_FORMAT);

  // unknown feature
  answer = ask([](oipb::Request& r) {
    auto* set = r.mutable_set_measurement_config();
    set->set_id(987654);
    set->set_config_name("ProjectPoint");
    set->set_saved(false);
  });
  checkAnswered(answer, oipb::REQUEST_KIND_SET_MEASUREMENT_CONFIG,
                oipb::ERROR_CODE_NO_FEATURE_WITH_ID);

  // the real thing
  answer = ask([id](oipb::Request& r) {
    auto* set = r.mutable_set_measurement_config();
    set->set_id(id);
    set->set_config_name("ProjectPoint");
    set->set_saved(false);
  });
  checkAnswered(answer, oipb::REQUEST_KIND_SET_MEASUREMENT_CONFIG,
                oipb::ERROR_CODE_NONE);

  answer = ask([id](oipb::Request& r) {
    r.mutable_get_measurement_config()->set_id(id);
  });
  checkAnswered(answer, oipb::REQUEST_KIND_GET_MEASUREMENT_CONFIG,
                oipb::ERROR_CODE_NONE);
  QCOMPARE(answer.response().get_measurement_config().id(), id);
  QCOMPARE(
      QString::fromStdString(
          answer.response().get_measurement_config().config().config().name()),
      QString("ProjectPoint"));
}

void RequestHandlerTest::addFeatures() {
  const auto countFeatures = [this] {
    return ask([](oipb::Request& r) { r.mutable_get_features(); })
        .response()
        .get_features()
        .features_size();
  };
  const int before = countFeatures();

  auto answer = ask([](oipb::Request& r) {
    auto* add = r.mutable_add_features();
    add->set_kind(oipb::FEATURE_KIND_PLANE);
    add->set_name("Pl");
    add->set_group("G");
    add->set_count(2);
    add->set_measurement_config("ProjectPoint");
  });
  checkAnswered(answer, oipb::REQUEST_KIND_ADD_FEATURES, oipb::ERROR_CODE_NONE);
  QCOMPARE(countFeatures(), before + 2);

  // name, group and count are required
  answer = ask([](oipb::Request& r) {
    auto* add = r.mutable_add_features();
    add->set_kind(oipb::FEATURE_KIND_PLANE);
    add->set_group("G");
    add->set_count(2);
  });
  checkAnswered(answer, oipb::REQUEST_KIND_ADD_FEATURES,
                oipb::ERROR_CODE_WRONG_FORMAT);

  // so is a kind that can be created
  for (const auto kind :
       {oipb::FEATURE_KIND_UNSPECIFIED, oipb::FEATURE_KIND_GROUP,
        oipb::FEATURE_KIND_POINT_GROUP, oipb::FEATURE_KIND_LEVEL}) {
    answer = ask([kind](oipb::Request& r) {
      auto* add = r.mutable_add_features();
      add->set_kind(kind);
      add->set_name("X");
      add->set_group("G");
      add->set_count(1);
    });
    checkAnswered(answer, oipb::REQUEST_KIND_ADD_FEATURES,
                  oipb::ERROR_CODE_WRONG_FORMAT);
  }
  QCOMPARE(countFeatures(), before + 2);
}

void RequestHandlerTest::aimNeedsASensor() {
  const int id = point->getFeature()->getId();

  auto answer = ask([](oipb::Request& r) { r.mutable_aim(); });
  checkAnswered(answer, oipb::REQUEST_KIND_AIM, oipb::ERROR_CODE_WRONG_FORMAT);

  answer = ask([id](oipb::Request& r) { r.mutable_aim()->set_id(id); });
  checkAnswered(answer, oipb::REQUEST_KIND_AIM,
                oipb::ERROR_CODE_NO_SENSOR_CONNECTED);

  answer = ask([id](oipb::Request& r) { r.mutable_measure()->set_id(id); });
  checkAnswered(answer, oipb::REQUEST_KIND_MEASURE,
                oipb::ERROR_CODE_NO_SENSOR_CONNECTED);

  answer = ask([](oipb::Request& r) { r.mutable_measure(); });
  checkAnswered(answer, oipb::REQUEST_KIND_MEASURE,
                oipb::ERROR_CODE_WRONG_FORMAT);
}

void RequestHandlerTest::watchwindowChecks() {
  // nothing to stop
  auto answer = ask([](oipb::Request& r) { r.mutable_stop_watchwindow(); });
  checkAnswered(answer, oipb::REQUEST_KIND_STOP_WATCHWINDOW,
                oipb::ERROR_CODE_NO_TASK);

  // the reading kind is required
  answer = ask([](oipb::Request& r) { r.mutable_start_watchwindow(); });
  checkAnswered(answer, oipb::REQUEST_KIND_START_WATCHWINDOW,
                oipb::ERROR_CODE_WRONG_FORMAT);

  // no active feature yet
  answer = ask([](oipb::Request& r) {
    r.mutable_start_watchwindow()->set_reading(oipb::READING_KIND_POLAR);
  });
  checkAnswered(answer, oipb::REQUEST_KIND_START_WATCHWINDOW,
                oipb::ERROR_CODE_NO_ACTIVE_FEATURE);

  // an active feature, but no sensor
  point->getFeature()->setActiveFeatureState(true);
  answer = ask([](oipb::Request& r) {
    r.mutable_start_watchwindow()->set_reading(oipb::READING_KIND_CARTESIAN);
  });
  checkAnswered(answer, oipb::REQUEST_KIND_START_WATCHWINDOW,
                oipb::ERROR_CODE_NO_SENSOR_CONNECTED);
}

void RequestHandlerTest::noJob() {
  OiRequestHandler empty;
  std::vector<OiRequestResponse> sent;
  QObject::connect(&empty, &OiRequestHandler::sendResponse, this,
                   [&sent](OiRequestResponse r) { sent.push_back(r); });

  oipb::Envelope envelope;
  envelope.set_request_id(5);
  envelope.mutable_request()->mutable_get_project();
  OiRequestResponse request;
  request.requesterId = 3;
  request.request = toStdBytes(bytesOf(envelope));
  QVERIFY(!empty.receiveRequest(request));

  QCOMPARE(sent.size(), 1);
  QCOMPARE(sent.front().requesterId, 3);
  QCOMPARE(static_cast<int>(sent.front().errorCode),
           static_cast<int>(OiRequestResponse::ErrorCode::eNoJob));
  oipb::Envelope reply;
  QVERIFY(reply.ParseFromArray(sent.front().response.data(),
                               static_cast<int>(sent.front().response.size())));
  QCOMPARE(reply.request_id(), 5);
  QCOMPARE(static_cast<int>(reply.response().error()),
           static_cast<int>(oipb::ERROR_CODE_NO_JOB));
  QCOMPARE(static_cast<int>(reply.response().request()),
           static_cast<int>(oipb::REQUEST_KIND_GET_PROJECT));
}

void RequestHandlerTest::garbageBytes() {
  const QList<QByteArray> inputs = {
      QByteArray("not a protobuf message at all"),
      QByteArray("\xff\xff\xff\xff\xff\xff", 6),
      QByteArray("\x0a\x80", 2),  // a length-delimited field cut short
      QByteArray(1000, '\x07'),
  };
  for (const auto& bytes : inputs) {
    const auto answer = sendBytes(bytes);
    QVERIFY2(!answer.accepted, bytes.toHex().constData());
    QCOMPARE(answer.sent, 1);
    QCOMPARE(static_cast<int>(answer.response().error()),
             static_cast<int>(oipb::ERROR_CODE_WRONG_FORMAT));
    QCOMPARE(static_cast<int>(answer.carrier.errorCode),
             static_cast<int>(OiRequestResponse::ErrorCode::eWrongFormat));
    QCOMPARE(answer.carrier.requesterId, 7);
  }
}

void RequestHandlerTest::notAnEnvelopeOfRequest() {
  // empty bytes parse, but hold nothing
  auto answer = sendBytes(QByteArray());
  QVERIFY(!answer.accepted);
  QCOMPARE(static_cast<int>(answer.response().error()),
           static_cast<int>(oipb::ERROR_CODE_WRONG_FORMAT));

  // an envelope that is a response or a notification is not a request
  oipb::Envelope envelope;
  envelope.mutable_response()->mutable_get_project();
  answer = send(envelope);
  QVERIFY(!answer.accepted);
  QCOMPARE(static_cast<int>(answer.response().error()),
           static_cast<int>(oipb::ERROR_CODE_WRONG_FORMAT));

  envelope.mutable_notification()->mutable_feature_set_changed();
  answer = send(envelope);
  QVERIFY(!answer.accepted);
  QCOMPARE(static_cast<int>(answer.response().error()),
           static_cast<int>(oipb::ERROR_CODE_WRONG_FORMAT));
}

void RequestHandlerTest::missingPayload() {
  oipb::Envelope envelope;
  envelope.set_request_id(8);
  envelope.mutable_request();  // a request that asks for nothing
  const auto answer = send(envelope);
  QVERIFY(!answer.accepted);
  checkAnswered(answer, oipb::REQUEST_KIND_UNSPECIFIED,
                oipb::ERROR_CODE_WRONG_FORMAT, 8);
}

void RequestHandlerTest::unknownRequest() {
  // a request from a newer protocol: a field this version has no member for
  oipb::Envelope envelope;
  envelope.set_request_id(9);
  auto* request = envelope.mutable_request();
  request->GetReflection()->MutableUnknownFields(request)->AddVarint(99, 1);
  const auto answer = send(envelope);
  QVERIFY(!answer.accepted);
  checkAnswered(answer, oipb::REQUEST_KIND_UNSPECIFIED,
                oipb::ERROR_CODE_UNKNOWN_REQUEST_TYPE, 9);
  QCOMPARE(static_cast<int>(answer.carrier.myRequestType),
           static_cast<int>(OiRequestResponse::RequestType::eUnknownRequest));
}

void RequestHandlerTest::toolRequestIsNotAnswered() {
  oipb::Envelope envelope;
  envelope.mutable_request()->mutable_tool()->set_data("anything");
  const auto answer = send(envelope);
  QVERIFY(answer.accepted);
  QCOMPARE(answer.sent, 0);
}

void RequestHandlerTest::notifications() {
  std::vector<OiRequestResponse> sent;
  QObject::connect(&handler, &OiRequestHandler::sendResponse, this,
                   [&sent](OiRequestResponse r) { sent.push_back(r); });
  const auto last = [&sent] {
    oipb::Envelope envelope;
    const auto& carrier = sent.back();
    [&] {
      QVERIFY(envelope.ParseFromArray(
          carrier.response.data(), static_cast<int>(carrier.response.size())));
    }();
    return envelope;
  };

  handler.sensorActionStarted("measure");
  QCOMPARE(sent.size(), 1);
  QCOMPARE(sent.back().requesterId, -1);
  QCOMPARE(
      static_cast<int>(sent.back().myRequestType),
      static_cast<int>(OiRequestResponse::RequestType::eSensorActionStarted));
  auto envelope = last();
  QCOMPARE(envelope.content_case(), oipb::Envelope::kNotification);
  QCOMPARE(envelope.request_id(), 0);
  QCOMPARE(QString::fromStdString(
               envelope.notification().sensor_action_started().name()),
           QString("measure"));

  handler.sensorActionFinished(true, "done");
  envelope = last();
  QVERIFY(envelope.notification().sensor_action_finished().success());
  QCOMPARE(QString::fromStdString(
               envelope.notification().sensor_action_finished().message()),
           QString("done"));

  handler.log("careful", MessageTypes::eWarningMessage);
  envelope = last();
  QCOMPARE(QString::fromStdString(envelope.notification().message_box().text()),
           QString("careful"));
  QCOMPARE(static_cast<int>(envelope.notification().message_box().kind()),
           static_cast<int>(oipb::MESSAGE_KIND_WARNING));

  handler.activeFeatureChanged();
  QCOMPARE(last().notification().payload_case(),
           oipb::Notification::kActiveFeatureChanged);
  handler.activeStationChanged();
  QCOMPARE(last().notification().payload_case(),
           oipb::Notification::kActiveStationChanged);
  handler.activeCoordinateSystemChanged();
  QCOMPARE(last().notification().payload_case(),
           oipb::Notification::kActiveCoordinateSystemChanged);
  handler.featureSetChanged();
  QCOMPARE(last().notification().payload_case(),
           oipb::Notification::kFeatureSetChanged);
  handler.featureAttributesChanged();
  QCOMPARE(last().notification().payload_case(),
           oipb::Notification::kFeatureAttributesChanged);
  QCOMPARE(sent.size(), 8);
}

void RequestHandlerTest::realTimeReadingWithoutWatchwindow() {
  std::vector<OiRequestResponse> sent;
  QObject::connect(&handler, &OiRequestHandler::sendResponse, this,
                   [&sent](OiRequestResponse r) { sent.push_back(r); });
  QVariantMap reading;
  reading["x"] = 1.0;
  reading["y"] = 2.0;
  reading["z"] = 3.0;
  handler.realTimeReading(reading);
  QCOMPARE(sent.size(), 0);
}

QTEST_APPLESS_MAIN(RequestHandlerTest)
#include "tst_requesthandler.moc"

// "Has the project changed since it was saved?" is a flag, not a comparison of
// content: every notification about saved content sets it, a save or a load
// clears it. These tests pin both directions: each kind of saved change marks
// the project dirty, and what the project file does not store (streams,
// connection state, working group, derived flags) does not.
#include <QPointer>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>
#include <atomic>
#include <functional>
#include <map>
#include <memory>
#include <vector>

#include "bundle_adjustment.h"
#include "choose_la_lib.h"
#include "controller.h"
#include "coordinate_system.h"
#include "feature_wrapper.h"
#include "fit_function.h"
#include "geometry/plane.h"
#include "geometry/point.h"
#include "group.h"
#include "oi_job.h"
#include "project_dirty_tracker.h"
#include "qt_convert.h"
#include "qt_executor.h"
#include "reading.h"
#include "sensor.h"
#include "station.h"
#include "trafo_param.h"

using namespace oi;

namespace {

class DirtyFit : public FitFunction, public Producer<Point> {
 public:
  DirtyFit() {
    metaData.name = "DirtyFit";
    metaData.pluginName = "Test Plugin";
    NeededElement points;
    points.typeOfElement = ElementTypes::ePointElement;
    points.infinite = true;
    points.roleName = "points";
    neededElements.push_back(points);
    applicableFor.push_back(FeatureTypes::ePointFeature);
  }
  std::expected<Point, std::string> exec() override {
    return std::unexpected(std::string("not used"));
  }
};

class DirtyBundle : public BundleAdjustment {
 public:
  DirtyBundle() {
    metaData.name = "DirtyBundle";
    metaData.pluginName = "Test Plugin";
  }
};

// a driver that is always connected and streams readings
class StreamingSensor : public Sensor {
 public:
  std::atomic<int> counter{0};
  bool isSensorAsync() const override { return false; }
  bool getConnectionState() override { return true; }
  bool getIsReadyForMeasurement() override { return true; }
  bool getIsBusy() override { return false; }
  std::map<std::string, std::string> getSensorStatus() override {
    return {{"a", "b"}};
  }
  std::vector<std::unique_ptr<Reading>> measure(
      const MeasurementConfig&) override {
    return {};
  }
  oi::ValueMap readingStream(const ReadingTypes&) override {
    QThread::msleep(1);
    oi::ValueMap m;
    m["n"] = ++this->counter;
    return m;
  }
  bool accept(const SensorFunctions&, const SensorAttributes&) override {
    return true;
  }
};

// a point the job accepts (it needs a name)
std::unique_ptr<Point> namedPoint(const std::string& name) {
  auto point = std::make_unique<Point>();
  point->setFeatureName(name);
  return point;
}

struct World {
  OiJob job;
  ProjectDirtyTracker tracker;
  Station* station = nullptr;
  CoordinateSystem* part = nullptr;
  CoordinateSystem* bundle = nullptr;
  Point* a = nullptr;
  Point* b = nullptr;
  Point* c = nullptr;  // carries a function
  Plane* plane = nullptr;
  Group* group = nullptr;
  TrafoParam* trafo = nullptr;
};

Point* addPoint(World& w, const std::string& name) {
  auto p = std::make_unique<Point>(Position(1.0, 2.0, 3.0));
  Point* raw = p.get();
  raw->setFeatureName(name);
  w.job.addFeature(std::move(p));
  return raw;
}

// one of everything the project stores; the tracker starts clean
void build(World& w) {
  auto s = std::make_unique<Station>("S1");
  w.station = s.get();
  w.job.addFeature(std::move(s));
  auto p = std::make_unique<CoordinateSystem>();
  w.part = p.get();
  w.part->setFeatureName("PART");
  w.job.addFeature(std::move(p));
  auto bs = std::make_unique<CoordinateSystem>(true);
  w.bundle = bs.get();
  w.bundle->setFeatureName("Bundle01");
  w.bundle->setBundlePlugin(std::make_unique<DirtyBundle>());
  w.job.addFeature(std::move(bs));
  w.a = addPoint(w, "A");
  w.b = addPoint(w, "B");
  auto cp = namedPoint("C");
  w.c = cp.get();
  w.c->addFunction(std::make_unique<DirtyFit>());
  w.job.addFeature(std::move(cp));
  auto pl = std::make_unique<Plane>();
  w.plane = pl.get();
  w.plane->setFeatureName("P1");
  w.job.addFeature(std::move(pl));
  auto g = std::make_unique<Group>();
  w.group = g.get();
  w.group->setFeatureName("G");
  w.job.addFeature(std::move(g));
  auto t = std::make_unique<TrafoParam>();
  w.trafo = t.get();
  w.trafo->setFeatureName("T1");
  w.trafo->setCoordinateSystems(w.station->getCoordinateSystem(), w.part);
  w.job.addFeature(std::move(t));
  w.station->setActiveStationState(true);
  w.tracker.attach(&w.job);
}

using Mutation = std::function<void(World&)>;

}  // namespace

Q_DECLARE_METATYPE(Mutation)

class ProjectDirtyTrackerTest : public QObject {
  Q_OBJECT

 private slots:
  void initTestCase();

  void attachedJobStartsClean();
  void eachSavedChangeMarksDirty_data();
  void eachSavedChangeMarksDirty();
  void markCleanClearsAndTheNextChangeMarksAgain();
  void undoEquivalentEditStillMarksDirty();
  void changesBeforeAttachingDoNotCount();
  void attachingToAnotherJobResetsAndForgetsTheFirst();
  void trackerSurvivesItsJob();
  void jobSurvivesItsTracker();

  void unsavedChangesKeepTheProjectClean_data();
  void unsavedChangesKeepTheProjectClean();
  void streamingAndConnectionStateKeepTheProjectClean();

  void controllerNewProjectIsClean();
  void controllerChangesSaveAndLoad();
  void controllerFailedSaveKeepsTheFlag();
  void controllerNamesTheJobAfterTheFile();
  void controllerMarkProjectDirtyForChangesOutsideTheJob();
};

void ProjectDirtyTrackerTest::initTestCase() {
  ChooseLALib::setLinearAlgebra(ChooseLALib::LinearAlgebraLibrary::Armadillo);
}

void ProjectDirtyTrackerTest::attachedJobStartsClean() {
  World w;
  build(w);
  QVERIFY(!w.tracker.isDirty());
  ProjectDirtyTracker unattached;
  QVERIFY(!unattached.isDirty());
}

void ProjectDirtyTrackerTest::eachSavedChangeMarksDirty_data() {
  QTest::addColumn<Mutation>("mutation");
  const auto row = [](const char* name, Mutation m) {
    QTest::newRow(name) << m;
  };

  // structure
  row("add feature", [](World& w) { addPoint(w, "N"); });
  row("add station",
      [](World& w) { w.job.addFeature(std::make_unique<Station>("S2")); });
  row("remove feature", [](World& w) { w.job.removeFeature(w.b->getId()); });
  row("rename", [](World& w) { w.a->setFeatureName("X"); });
  row("group of a feature", [](World& w) { w.a->setGroupName("G2"); });
  row("comment", [](World& w) { w.a->setComment("note"); });
  row("active feature", [](World& w) { w.a->setActiveFeatureState(true); });
  row("active coordinate system",
      [](World& w) { w.part->setActiveCoordinateSystemState(true); });
  row("active station", [](World& w) {
    w.job.addFeature(std::make_unique<Station>("S2"));
    w.job.getStationsList().back()->setActiveStationState(true);
  });
  row("group members",
      [](World& w) { w.group->addMember(w.job.getFeatureById(w.a->getId())); });

  // geometries
  row("point position",
      [](World& w) { w.a->setPoint(Position(4.0, 5.0, 6.0)); });
  row("plane", [](World& w) {
    w.plane->setPlane(Position(0.0, 0.0, 1.0), Direction(0.0, 0.0, 1.0));
  });
  row("measurement intent", [](World& w) {
    w.a->addIntent(MeasurementIntent(w.c->getId(), "points"));
  });
  row("common state", [](World& w) { w.a->setCommonState(true); });
  row("statistic", [](World& w) { w.a->setStatistic(Statistic()); });
  row("measurement config of a geometry", [](World& w) {
    w.a->setMeasurementConfig(
        MeasurementConfigKey("cfg", ConfigTypes::eProjectConfig));
  });
  row("nominal system", [](World& w) { w.a->setNominalSystem(w.part); });

  // functions
  row("function added to a feature",
      [](World& w) { w.b->addFunction(std::make_unique<DirtyFit>()); });
  row("function input element", [](World& w) {
    InputElement input(w.a->getId());
    input.typeOfElement = ElementTypes::ePointElement;
    input.point = w.a;
    input.geometry = w.a;
    w.c->getFunctions().front()->addInputElement(input, 0);
  });
  row("function use state", [](World& w) {
    InputElement input(w.a->getId());
    input.typeOfElement = ElementTypes::ePointElement;
    input.point = w.a;
    input.geometry = w.a;
    w.c->getFunctions().front()->addInputElement(input, 0);
    w.tracker.markClean();
    w.c->getFunctions().front()->setShouldBeUsed(0, w.a->getId(), false);
  });
  row("function pipeline", [](World& w) {
    w.c->getFunctions().front()->setPipeline(0, std::vector<PipelineStage>{});
  });
  row("function fixed parameter", [](World& w) {
    w.c->getFunctions().front()->fixParameter(
        FixedParameter(GeometryParameters::eUnknownX));
  });

  // transformations and systems
  row("trafo used", [](World& w) { w.trafo->setIsUsed(true); });
  row("trafo datum", [](World& w) { w.trafo->setIsDatumTrafo(true); });
  row("trafo parameters", [](World& w) {
    OiVec rotation(3);
    OiVec translation(3);
    OiVec scale(3);
    scale.setAt(0, 1.0);
    scale.setAt(1, 1.0);
    scale.setAt(2, 1.0);
    w.trafo->setTransformationParameters(rotation, translation, scale);
  });
  row("coordinate system frame", [](World& w) {
    w.part->setCoordinateSystem(Position(1, 2, 3), Direction(1, 0, 0),
                                Direction(0, 1, 0), Direction(0, 0, 1));
  });
  row("expansion origin",
      [](World& w) { w.part->setExpansionOrigin(Position(1, 1, 1)); });
  row("bundle template", [](World& w) { w.bundle->setBundleTemplate("{}"); });
  row("bundle input stations",
      [](World& w) { w.bundle->getBundlePlugin()->setInputStations({}); });
  row("bundle base station", [](World& w) {
    w.bundle->getBundlePlugin()->setBaseStation(BundleStation());
  });

  // measurements and the sensor
  row("measurement", [](World& w) {
    ReadingCartesian r;
    r.isValid = true;
    std::vector<std::unique_ptr<Reading>> readings;
    readings.push_back(std::make_unique<Reading>(r));
    w.job.addMeasurementResults(w.a->getId(), std::move(readings));
  });
  row("remove observations",
      [](World& w) { w.job.removeObservations(w.a->getId()); });
  row("station sensor", [](World& w) { w.station->resetSensor(); });
}

void ProjectDirtyTrackerTest::eachSavedChangeMarksDirty() {
  QFETCH(Mutation, mutation);
  World w;
  build(w);
  QVERIFY(!w.tracker.isDirty());
  mutation(w);
  QVERIFY(w.tracker.isDirty());
}

void ProjectDirtyTrackerTest::markCleanClearsAndTheNextChangeMarksAgain() {
  World w;
  build(w);
  w.a->setComment("one");
  QVERIFY(w.tracker.isDirty());
  w.tracker.markClean();  // "saved"
  QVERIFY(!w.tracker.isDirty());
  w.a->setComment("two");
  QVERIFY(w.tracker.isDirty());
  w.tracker.markClean();
  w.tracker.markDirty();  // a change outside the job
  QVERIFY(w.tracker.isDirty());
}

void ProjectDirtyTrackerTest::undoEquivalentEditStillMarksDirty() {
  // flag semantics, not content equality: renaming and renaming back leaves
  // the project marked as changed
  World w;
  build(w);
  w.a->setFeatureName("Renamed");
  w.a->setFeatureName("A");
  QVERIFY(w.tracker.isDirty());
}

void ProjectDirtyTrackerTest::changesBeforeAttachingDoNotCount() {
  OiJob job;
  job.addFeature(namedPoint("N"));
  ProjectDirtyTracker tracker;
  tracker.attach(&job);
  QVERIFY(!tracker.isDirty());
}

void ProjectDirtyTrackerTest::attachingToAnotherJobResetsAndForgetsTheFirst() {
  OiJob first;
  OiJob second;
  ProjectDirtyTracker tracker;
  tracker.attach(&first);
  first.addFeature(namedPoint("N"));
  QVERIFY(tracker.isDirty());

  tracker.attach(&second);
  QVERIFY(!tracker.isDirty());
  first.addFeature(namedPoint("N2"));  // no longer tracked
  QVERIFY(!tracker.isDirty());
  second.addFeature(namedPoint("N3"));
  QVERIFY(tracker.isDirty());

  tracker.detach();
  tracker.markClean();
  second.addFeature(namedPoint("N4"));
  QVERIFY(!tracker.isDirty());
}

void ProjectDirtyTrackerTest::trackerSurvivesItsJob() {
  ProjectDirtyTracker tracker;
  {
    OiJob job;
    tracker.attach(&job);
    job.addFeature(namedPoint("N"));
  }
  QVERIFY(tracker.isDirty());  // the flag outlives the job
  tracker.markClean();
  tracker.detach();  // must not touch the dead job
  QVERIFY(!tracker.isDirty());
}

void ProjectDirtyTrackerTest::jobSurvivesItsTracker() {
  OiJob job;
  {
    ProjectDirtyTracker tracker;
    tracker.attach(&job);
  }
  job.addFeature(namedPoint("N"));  // no dangling observer
}

void ProjectDirtyTrackerTest::unsavedChangesKeepTheProjectClean_data() {
  QTest::addColumn<Mutation>("action");
  const auto row = [](const char* name, Mutation m) {
    QTest::newRow(name) << m;
  };
  row("working group", [](World& w) { w.job.setActiveGroup("whatever"); });
  row("recalculation bookkeeping", [](World& w) {
    w.job.featureRecalculated(w.a->getId());
    w.job.featuresRecalculated();
    w.job.trafoParamRecalculated(w.trafo->getId());
  });
  row("messages", [](World& w) {
    w.job.sendMessage("hello", MessageTypes::eInformationMessage);
  });
  row("solved flag (derived)", [](World& w) { w.a->setIsSolved(true); });
  row("updated flag (not saved)", [](World& w) { w.a->setIsUpdated(true); });
}

void ProjectDirtyTrackerTest::unsavedChangesKeepTheProjectClean() {
  QFETCH(Mutation, action);
  World w;
  build(w);
  action(w);
  QVERIFY(!w.tracker.isDirty());
}

void ProjectDirtyTrackerTest::streamingAndConnectionStateKeepTheProjectClean() {
  QtExecutor executor;
  World w;
  build(w);
  w.job.setExecutor(&executor);
  w.station->setSensor(std::make_unique<StreamingSensor>());
  QVERIFY(w.tracker.isDirty());  // choosing the sensor is saved
  w.tracker.markClean();

  w.station->connectSensor();  // connection status results
  w.station->startReadingStream();
  w.station->startStatusMonitoringStream();
  QTest::qWait(300);  // real-time readings and status arrive on the UI thread
  w.station->stopReadingStream();
  w.station->stopStatusMonitoringStream();
  QTest::qWait(50);

  QVERIFY(!w.tracker.isDirty());
}

void ProjectDirtyTrackerTest::controllerNewProjectIsClean() {
  Controller controller;
  QVERIFY(!controller.isProjectDirty());  // no project yet
  controller.createDefaultJob();
  QVERIFY(!controller.isProjectDirty());  // station, system, bundle system
}

void ProjectDirtyTrackerTest::controllerChangesSaveAndLoad() {
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  const QString file = dir.filePath("project.oip");

  Controller controller;
  const oi::Guarded<OiJob> job = controller.createDefaultJob();
  QVERIFY(!controller.isProjectDirty());

  job->addFeature(namedPoint("N"));
  QVERIFY(controller.isProjectDirty());

  QVERIFY(controller.saveProject(file));  // save as: clean afterwards
  QVERIFY(QFile::exists(file));
  QVERIFY(!controller.isProjectDirty());

  job->getFeaturesByName("PART").front()->getFeature()->setComment("edited");
  QVERIFY(controller.isProjectDirty());
  QVERIFY(controller.saveProject());  // save: clean again
  QVERIFY(!controller.isProjectDirty());

  // a loaded project is clean, however much the load touches the job
  controller.markProjectDirty();
  controller.loadProject(file);
  QVERIFY(!controller.isProjectDirty());

  // and it tracks the loaded job, not the old one
  QVERIFY(!controller.createDefaultJob().isNull());
  QVERIFY(!controller.isProjectDirty());
}

void ProjectDirtyTrackerTest::controllerFailedSaveKeepsTheFlag() {
  Controller controller;
  const oi::Guarded<OiJob> job = controller.createDefaultJob();
  job->addFeature(namedPoint("N"));
  QVERIFY(controller.isProjectDirty());
  QVERIFY(!controller.saveProject("/nonexistent-directory/project.oip"));
  QVERIFY(controller.isProjectDirty());
}

void ProjectDirtyTrackerTest::
    controllerMarkProjectDirtyForChangesOutsideTheJob() {
  Controller controller;
  controller.createDefaultJob();
  QVERIFY(!controller.isProjectDirty());
  controller.markProjectDirty();
  QVERIFY(controller.isProjectDirty());
  controller.markProjectClean();
  QVERIFY(!controller.isProjectDirty());
}

void ProjectDirtyTrackerTest::controllerNamesTheJobAfterTheFile() {
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  const QString first = dir.filePath("first.oip");
  const QString second = dir.filePath("second.oip");

  Controller controller;
  const oi::Guarded<OiJob> job = controller.createDefaultJob();
  QVERIFY(job->getJobName().empty());  // never saved: no name

  QVERIFY(controller.saveProject(first));
  QCOMPARE(QString::fromStdString(job->getJobName()), QString("first.oip"));
  QVERIFY(!controller.isProjectDirty());

  // the file holds neither the directory nor the name
  QFile file(first);
  QVERIFY(file.open(QIODevice::ReadOnly));
  QByteArray bytes = file.readAll();
  file.close();
  QVERIFY(!bytes.contains(dir.path().toUtf8()));
  QVERIFY(!bytes.contains("first.oip"));

  // save as: the job is renamed, and the rename alone is not a change
  QVERIFY(controller.saveProject(second));
  QCOMPARE(QString::fromStdString(job->getJobName()), QString("second.oip"));
  QCOMPARE(fromPath(job->getJobFile()), second);
  QVERIFY(!controller.isProjectDirty());

  // loading names the job after the file, whatever it was called when saved
  oi::Guarded<OiJob> reloaded;
  QObject::connect(&controller, &Controller::setCurrentJob,
                   [&reloaded](const oi::Guarded<OiJob>& j) { reloaded = j; });
  controller.loadProject(first);
  QVERIFY(!controller.isProjectDirty());
  QVERIFY(!reloaded.isNull());
  QCOMPARE(QString::fromStdString(reloaded->getJobName()),
           QString("first.oip"));
  QCOMPARE(fromPath(reloaded->getJobFile()), first);
}

QTEST_GUILESS_MAIN(ProjectDirtyTrackerTest)
#include "tst_project_dirty_tracker.moc"

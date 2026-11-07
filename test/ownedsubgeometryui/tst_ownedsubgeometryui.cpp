#include <QPointer>
#include <QtTest>
#include <map>

#include "chooselalib.h"
#include "controller.h"
#include "featurewrapper.h"
#include "geometry/line.h"
#include "geometry/plane.h"
#include "oijob.h"
#include "plugin/function/constructfunction.h"
#include "plugin/function/producertransform.h"
#include "pluginrepository.h"

using namespace oi;
using namespace oi::math;

namespace {

// Test-local producer, same shape as openindy-default-plugin's
// tst_ownedplaneaccessors.cpp SetPlaneFunction: gives the owning Plane a
// real producer so plane->recalc() can actually re-derive its value (a
// Plane with no producerFunction is a no-op on recalc since Stage 7d -
// Feature::recalc() returns immediately if producerFunction has no value).
// Needed to prove the owned line tracks its owner on recompute, not just
// at creation time.
class SetPlaneFunction : public ConstructFunction, public Producer<Plane> {
 public:
  explicit SetPlaneFunction() : ConstructFunction() {
    this->metaData.name = "SetPlaneFunction";
    this->applicableFor.push_back(FeatureTypes::ePlaneFeature);
  }
  Position position;
  Direction direction;
  std::expected<Plane, std::string> exec() override {
    Plane plane;
    plane.setPlane(position, direction);
    return plane;
  }
};

}  // namespace

/*!
 * UI verification checkpoint: proves Controller::generateOwnedNormalLine()/
 * generateOwnedOriginPoint() - the new feature-table context-menu actions
 * (mainwindow.cpp's on_tableView_features_customContextMenuRequested) -
 * actually reach OiJob::addOwnedSubGeometry through the real
 * PluginRepository/PluginManager path, not just that the underlying
 * mechanism works (tst_ownedplaneaccessors.cpp in openindy-default-plugin
 * already proves that at the API level with a directly-constructed
 * function). This is the first test in the openindy app repo to call
 * PluginRepository::init() for a real scan rather than ModelManager::
 * testInit()'s fake injection seam - deliberate, since resolving
 * "LineFromPlaneNormal"/"PointFromPlaneOrigin" to a real, working Function
 * instance is exactly what's being verified.
 */
class OwnedSubGeometryUiTest : public QObject {
  Q_OBJECT

 private slots:
  void initTestCase();

  void generateNormalLineActionCreatesBoundReadOnlyLine();
  void generateOriginPointActionCreatesBoundReadOnlyPoint();
  void ownedLineTracksItsOwnerOnRecompute();
  void actionDoesNothingWithoutASolvedActivePlane();

 private:
  // Controller::setJob()/createDefaultJob() are private slots (Qt's usual
  //"internal reaction, not a public API" convention in this codebase -
  // see OiJob::setActiveFeature below), so a test drives them the same way
  // the app itself does: through the one public entry point,
  // createDefaultJob(), then adds its own feature into the job that
  // returns. Sets up a real station+system, which this test doesn't need
  // but costs nothing to have.
  oi::Guarded<OiJob> makeJobWithPlane(
      Controller& controller, oi::Guarded<FeatureWrapper>& outPlaneWrapper,
      oi::Guarded<SetPlaneFunction>& outPlaneFn, const Position& position,
      const Direction& normal);
  oi::Guarded<Line> findOnlyLine(const oi::Guarded<OiJob>& job);
  oi::Guarded<Point> findOnlyGeneratedPoint(
      const oi::Guarded<OiJob>& job,
      const oi::Guarded<FeatureWrapper>& excludingOwner);
};

void OwnedSubGeometryUiTest::initTestCase() {
  // without this every OiMat operation dereferences a null linear-algebra
  // pointer - same requirement as every other suite in this project
  ChooseLALib::setLinearAlgebra(ChooseLALib::LinearAlgebraLibrary::Armadillo);

  // real scan against build/bin/plugins/ (deployed there by CMakeLists.txt's
  // post-build copy step, same directory layout the real app uses) - not
  // the fake sys::Function injection dialogstest.cpp uses, because this
  // test specifically needs a real, loadable LineFromPlaneNormal/
  // PointFromPlaneOrigin to prove the context-menu action's plugin lookup
  // actually works end to end
  PluginRepository::init();
}

oi::Guarded<OiJob> OwnedSubGeometryUiTest::makeJobWithPlane(
    Controller& controller, oi::Guarded<FeatureWrapper>& outPlaneWrapper,
    oi::Guarded<SetPlaneFunction>& outPlaneFn, const Position& position,
    const Direction& normal) {
  const oi::Guarded<OiJob>& job = controller.createDefaultJob();

  auto own_plane = std::make_unique<oi::Plane>();
  oi::Guarded<oi::Plane> plane = own_plane.get();
  oi::Guarded<oi::FeatureWrapper> planeWrapper = plane->getFeatureWrapper();
  plane->setFeatureName("Plane1");
  job->addFeature(std::move(own_plane));

  oi::Guarded<SetPlaneFunction> planeFn = new SetPlaneFunction();
  planeFn->position = position;
  planeFn->direction = normal;
  plane->addFunction(planeFn);
  plane->recalc();

  // the public way to make a feature active - OiJob::setActiveFeature is a
  // private slot reacting to this signal, same convention as Controller's
  // own setJob()/createDefaultJob() split above
  plane->setActiveFeatureState(true);

  outPlaneWrapper = planeWrapper;
  outPlaneFn = planeFn;
  return job;
}

oi::Guarded<Line> OwnedSubGeometryUiTest::findOnlyLine(
    const oi::Guarded<OiJob>& job) {
  oi::Guarded<Line> found;
  foreach (const oi::Guarded<FeatureWrapper>& wrapper,
           job->getGeometriesList()) {
    if (!wrapper.isNull() && !wrapper->getLine().isNull()) {
      found = wrapper->getLine();
    }
  }
  return found;
}

oi::Guarded<Point> OwnedSubGeometryUiTest::findOnlyGeneratedPoint(
    const oi::Guarded<OiJob>& job,
    const oi::Guarded<FeatureWrapper>& excludingOwner) {
  oi::Guarded<Point> found;
  foreach (const oi::Guarded<FeatureWrapper>& wrapper,
           job->getGeometriesList()) {
    if (!wrapper.isNull() && !wrapper->getPoint().isNull() &&
        wrapper != excludingOwner) {
      found = wrapper->getPoint();
    }
  }
  return found;
}

void OwnedSubGeometryUiTest::
    generateNormalLineActionCreatesBoundReadOnlyLine() {
  Controller controller;
  oi::Guarded<FeatureWrapper> planeWrapper;
  oi::Guarded<SetPlaneFunction> planeFn;
  oi::Guarded<OiJob> job =
      this->makeJobWithPlane(controller, planeWrapper, planeFn,
                             Position(1.0, 2.0, 3.0), Direction(0.0, 0.0, 1.0));

  // the real, production entry point the new context-menu action calls
  controller.generateOwnedNormalLine();

  oi::Guarded<Line> line = this->findOnlyLine(job);
  QVERIFY2(!line.isNull(),
           "generateOwnedNormalLine() must create a new Line feature");

  QVERIFY(line->getOwnerDescription().has_value());
  QVERIFY(line->getOwnerDescription()->contains("Plane1"));
  QVERIFY(line->getOwnerDescription()->contains("LineFromPlaneNormal"));

  QCOMPARE(line->getPosition().getVector().getAt(0), 1.0);
  QCOMPARE(line->getPosition().getVector().getAt(2), 3.0);
  QCOMPARE(line->getDirection().getVector().getAt(2), 1.0);

  // read-only enforcement: an edit attempt must not take effect
  std::map<GeometryParameters, double> edit;
  edit.insert_or_assign(GeometryParameters::eUnknownX, 99.0);
  line->setUnknownParameters(edit);
  QCOMPARE(line->getPosition().getVector().getAt(0), 1.0);
}

void OwnedSubGeometryUiTest::
    generateOriginPointActionCreatesBoundReadOnlyPoint() {
  Controller controller;
  oi::Guarded<FeatureWrapper> planeWrapper;
  oi::Guarded<SetPlaneFunction> planeFn;
  oi::Guarded<OiJob> job =
      this->makeJobWithPlane(controller, planeWrapper, planeFn,
                             Position(4.0, 5.0, 6.0), Direction(0.0, 1.0, 0.0));

  controller.generateOwnedOriginPoint();

  oi::Guarded<Point> point = this->findOnlyGeneratedPoint(job, planeWrapper);
  QVERIFY2(!point.isNull(),
           "generateOwnedOriginPoint() must create a new Point feature");

  QVERIFY(point->getOwnerDescription().has_value());
  QVERIFY(point->getOwnerDescription()->contains("Plane1"));
  QVERIFY(point->getOwnerDescription()->contains("PointFromPlaneOrigin"));

  QCOMPARE(point->getPosition().getVector().getAt(0), 4.0);
  QCOMPARE(point->getPosition().getVector().getAt(1), 5.0);
  QCOMPARE(point->getPosition().getVector().getAt(2), 6.0);
}

void OwnedSubGeometryUiTest::ownedLineTracksItsOwnerOnRecompute() {
  Controller controller;
  oi::Guarded<FeatureWrapper> planeWrapper;
  oi::Guarded<SetPlaneFunction> planeFn;
  oi::Guarded<OiJob> job =
      this->makeJobWithPlane(controller, planeWrapper, planeFn,
                             Position(0.0, 0.0, 0.0), Direction(0.0, 0.0, 1.0));

  controller.generateOwnedNormalLine();

  oi::Guarded<Line> line = this->findOnlyLine(job);
  QVERIFY(!line.isNull());
  QCOMPARE(line->getDirection().getVector().getAt(0), 0.0);

  // move the plane by giving its own producer a new value and recalcing -
  // Geometry::recalc() only dirties owned children when the owner's value
  // actually changed (domain-model.md S1/S6), so this is the real
  // regeneration path, not a second mechanism
  planeFn->direction = Direction(1.0, 0.0, 0.0);
  planeWrapper->getGeometry()->recalc();
  line->recalc();

  QCOMPARE(line->getDirection().getVector().getAt(0), 1.0);
}

void OwnedSubGeometryUiTest::actionDoesNothingWithoutASolvedActivePlane() {
  // no active feature at all
  Controller controller;
  const oi::Guarded<OiJob>& job = controller.createDefaultJob();
  controller.generateOwnedNormalLine();
  QVERIFY2(job->getGeometriesList().empty(),
           "no active feature - the action must be a silent no-op");

  // an active feature that is not a solved Plane
  auto own_point = std::make_unique<oi::Point>();
  oi::Guarded<oi::Point> point = own_point.get();
  oi::Guarded<oi::FeatureWrapper> pointWrapper = point->getFeatureWrapper();
  point->setFeatureName("SomePoint");
  job->addFeature(std::move(own_point));
  point->setActiveFeatureState(true);

  controller.generateOwnedNormalLine();
  QCOMPARE(job->getGeometriesList().size(), 1);
}

// GUILESS, not APPLESS: PluginRepository::init() needs
// qApp->applicationDirPath() to resolve the real plugins/ directory next to the
// test binary, which requires a live QCoreApplication - but nothing here
// constructs a QWidget, so a full QApplication/display platform plugin is
// unnecessary
QTEST_GUILESS_MAIN(OwnedSubGeometryUiTest)
#include "tst_ownedsubgeometryui.moc"

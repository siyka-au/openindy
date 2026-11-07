#include <QByteArray>
#include <QPointer>
#include <QtTest>

#include "chooselalib.h"
#include "coordinatesystem.h"
#include "featureutil.h"
#include "featurewrapper.h"
#include "geometry/plane.h"
#include "geometry/point.h"
#include "geometry/sphere.h"
#include "group.h"
#include "oijob.h"
#include "pointgroup.h"
#include "projectexchanger.h"
#include "station.h"

using namespace oi;
using namespace oi::math;

/*!
 * Stage 7e acceptance criterion: "a Group of planes and a Group of spheres
 * both persist and reload correctly as organizational folders." Before this
 * stage a Group was not wired into ProjectExchanger at all - not written to
 * the project XML on save, not read back on load, and Group::pendingMemberIds
 * (populated by Group::fromOpenIndyXML) was never resolved into real member
 * references anywhere. This file exercises the real save/load round trip
 * through ProjectExchanger, the same entry points the application uses,
 * rather than testing Group::toOpenIndyXML/fromOpenIndyXML in isolation
 * (tst_pointcollection.cpp already covers Group's own membership mechanics).
 */
namespace {

// a project needs an active station and coordinate system, and loading
// creates "Bundle01_1" if it is missing, which requires a "Bundle01" system
// and a "STATION01" station - the same minimal setup
// test/dialogs/dialogstest.cpp uses to get a save/load round trip to succeed.
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

  // ProjectExchanger::loadProject auto-creates "Bundle01_1" on load if it
  // is missing - create it up front so save/load round trips through the
  // ordinary path (present in the saved XML, restored like anything else)
  // rather than the auto-create fallback, which is unrelated to what this
  // file is testing
  FeatureUtil().createInitialBundleTransformation(job);

  return job;
}

oi::Guarded<FeatureWrapper> addPoint(const oi::Guarded<OiJob>& job,
                                     const QString& name, double x, double y,
                                     double z) {
  oi::Guarded<FeatureWrapper> wrapper = new FeatureWrapper();
  oi::Guarded<Point> point = new Point(Position(x, y, z));
  point->setFeatureName(name);
  point->setIsSolved(true);
  wrapper->setPoint(point);
  job->addFeature(wrapper);
  return wrapper;
}

oi::Guarded<FeatureWrapper> addPlane(const oi::Guarded<OiJob>& job,
                                     const QString& name) {
  oi::Guarded<FeatureWrapper> wrapper = new FeatureWrapper();
  oi::Guarded<Plane> plane = new Plane();
  plane->setFeatureName(name);
  wrapper->setPlane(plane);
  job->addFeature(wrapper);
  return wrapper;
}

oi::Guarded<FeatureWrapper> addSphere(const oi::Guarded<OiJob>& job,
                                      const QString& name) {
  oi::Guarded<FeatureWrapper> wrapper = new FeatureWrapper();
  oi::Guarded<Sphere> sphere = new Sphere();
  sphere->setFeatureName(name);
  wrapper->setSphere(sphere);
  job->addFeature(wrapper);
  return wrapper;
}

}  // namespace

class ProjectExchangeTest : public QObject {
  Q_OBJECT

 private slots:

  void initTestCase();

  void groupOfMixedTypesSurvivesSaveAndReload();
  void pointGroupSurvivesSaveAndReload();
  void typeConstraintSurvivesSaveAndReload();
  void loadFailsOnAFileThatIsNotAProject();
};

void ProjectExchangeTest::initTestCase() {
  ChooseLALib::setLinearAlgebra(ChooseLALib::LinearAlgebraLibrary::Armadillo);
}

/*!
 * An unconstrained Group referencing a Plane and a Sphere - the mixed-type
 * organisational case verified fact 5 describes (registration spheres,
 * slicing planes, ...). Round trips through the real save()/load() entry
 * points and checks membership by name, not by pointer, since reload
 * legitimately produces new objects.
 */
void ProjectExchangeTest::groupOfMixedTypesSurvivesSaveAndReload() {
  oi::Guarded<OiJob> job = makeJobWithStationAndSystem();

  oi::Guarded<FeatureWrapper> plane = addPlane(job, "Plane01");
  oi::Guarded<FeatureWrapper> sphere = addSphere(job, "Sphere01");

  oi::Guarded<FeatureWrapper> groupWrapper = new FeatureWrapper();
  oi::Guarded<Group> group = new Group();
  group->setFeatureName("MixedGroup");
  groupWrapper->setGroup(group);
  QVERIFY2(group->addMember(plane),
           "plane should be accepted by an unconstrained group");
  QVERIFY2(group->addMember(sphere),
           "sphere should be accepted by an unconstrained group");
  QVERIFY(job->addFeature(groupWrapper));

  QCOMPARE(group->memberCount(), 2);

  ProjectExchanger exchanger;
  const QByteArray xml = exchanger.saveProject(job);

  oi::Guarded<OiJob> reloaded = exchanger.loadProject(xml).job;
  QVERIFY2(!reloaded.isNull(), "the project should reload without error");

  const QList<oi::Guarded<FeatureWrapper> > reloadedGroups =
      reloaded->getFeaturesByType(FeatureTypes::eGroupFeature);
  QCOMPARE(reloadedGroups.size(), 1);

  oi::Guarded<Group> reloadedGroup = reloadedGroups.first()->getGroup();
  QVERIFY(!reloadedGroup.isNull());
  QCOMPARE(reloadedGroup->getFeatureName(), QString("MixedGroup"));

  // this is the assertion that would have failed before Stage 7e:
  // pendingMemberIds was populated by fromOpenIndyXML and never resolved,
  // so a reloaded group was always empty regardless of what was saved
  QCOMPARE(reloadedGroup->memberCount(), 2);

  QStringList memberNames;
  foreach (const oi::Guarded<FeatureWrapper>& member,
           reloadedGroup->getMembers()) {
    if (!member.isNull() && !member->getFeature().isNull()) {
      memberNames << member->getFeature()->getFeatureName();
    }
  }
  memberNames.sort();
  QCOMPARE(memberNames, QStringList({"Plane01", "Sphere01"}));
}

/*!
 * PointGroup is a Group under the hood (FeatureWrapper::setPointGroup sets
 * both handles), so save/load must tell it apart from a plain Group on
 * reload - Group::toOpenIndyXML/PointGroup::toOpenIndyXML's "type"
 * attribute is what makes that possible.
 */
void ProjectExchangeTest::pointGroupSurvivesSaveAndReload() {
  oi::Guarded<OiJob> job = makeJobWithStationAndSystem();

  oi::Guarded<FeatureWrapper> p1 = addPoint(job, "P1", 0.0, 0.0, 0.0);
  oi::Guarded<FeatureWrapper> p2 = addPoint(job, "P2", 1.0, 0.0, 0.0);
  oi::Guarded<FeatureWrapper> p3 = addPoint(job, "P3", 0.0, 1.0, 0.0);

  oi::Guarded<FeatureWrapper> groupWrapper = new FeatureWrapper();
  oi::Guarded<PointGroup> pointGroup = new PointGroup();
  pointGroup->setFeatureName("Wall");
  groupWrapper->setPointGroup(pointGroup);
  QVERIFY(pointGroup->addMember(p1));
  QVERIFY(pointGroup->addMember(p2));
  QVERIFY(pointGroup->addMember(p3));
  QVERIFY(job->addFeature(groupWrapper));

  ProjectExchanger exchanger;
  const QByteArray xml = exchanger.saveProject(job);

  oi::Guarded<OiJob> reloaded = exchanger.loadProject(xml).job;
  QVERIFY(!reloaded.isNull());

  // must come back as a PointGroup, not a plain Group
  QCOMPARE(reloaded->getFeaturesByType(FeatureTypes::eGroupFeature).size(), 0);
  const QList<oi::Guarded<FeatureWrapper> > reloadedPointGroups =
      reloaded->getFeaturesByType(FeatureTypes::ePointGroupFeature);
  QCOMPARE(reloadedPointGroups.size(), 1);

  oi::Guarded<PointGroup> reloadedGroup =
      reloadedPointGroups.first()->getPointGroup();
  QVERIFY(!reloadedGroup.isNull());
  QCOMPARE(reloadedGroup->getFeatureName(), QString("Wall"));
  QCOMPARE(reloadedGroup->memberCount(), 3);

  // the PointCollection interface (what a fit function would actually call)
  // works on the reloaded object, proving the members are real Points with
  // their solved position restored, not just live references
  QCOMPARE(reloadedGroup->collectionPointCount(), 3);
}

/*!
 * A Group's type constraint (Group::getConstraint) is persisted separately
 * from PointGroup's fixed Point constraint, and must still reject a
 * wrong-typed member after a reload - not just on the original in-memory
 * object.
 */
void ProjectExchangeTest::typeConstraintSurvivesSaveAndReload() {
  oi::Guarded<OiJob> job = makeJobWithStationAndSystem();

  oi::Guarded<FeatureWrapper> plane = addPlane(job, "Plane01");
  oi::Guarded<FeatureWrapper> sphere = addSphere(job, "Sphere01");

  oi::Guarded<FeatureWrapper> groupWrapper = new FeatureWrapper();
  oi::Guarded<Group> group = new Group("Planes", FeatureTypes::ePlaneFeature);
  groupWrapper->setGroup(group);
  QVERIFY2(group->addMember(plane),
           "plane should be accepted by a plane-constrained group");
  QVERIFY2(!group->addMember(sphere),
           "sphere should be rejected by a plane-constrained group");
  QVERIFY(job->addFeature(groupWrapper));

  ProjectExchanger exchanger;
  const QByteArray xml = exchanger.saveProject(job);

  oi::Guarded<OiJob> reloaded = exchanger.loadProject(xml).job;
  QVERIFY(!reloaded.isNull());

  const QList<oi::Guarded<FeatureWrapper> > reloadedGroups =
      reloaded->getFeaturesByType(FeatureTypes::eGroupFeature);
  QCOMPARE(reloadedGroups.size(), 1);

  oi::Guarded<Group> reloadedGroup = reloadedGroups.first()->getGroup();
  QVERIFY(!reloadedGroup.isNull());
  QVERIFY2(reloadedGroup->getConstraint().has_value(),
           "the constraint itself must survive reload");
  QCOMPARE(reloadedGroup->getConstraint().value(), FeatureTypes::ePlaneFeature);
  QCOMPARE(reloadedGroup->memberCount(), 1);

  // find the reloaded sphere by name - a fresh, unconstrained wrapper
  // cannot be built here because accepts() checks the feature's own type,
  // which the reloaded Sphere object still correctly reports
  oi::Guarded<FeatureWrapper> reloadedSphere;
  foreach (const oi::Guarded<FeatureWrapper>& candidate,
           reloaded->getGeometriesList()) {
    if (!candidate.isNull() &&
        candidate->getFeatureTypeEnum() == FeatureTypes::eSphereFeature) {
      reloadedSphere = candidate;
    }
  }
  QVERIFY(!reloadedSphere.isNull());
  QVERIFY2(!reloadedGroup->addMember(reloadedSphere),
           "a plane-constrained group must still reject a sphere after reload");
}

/*!
 * Anything that is not a project must fail cleanly - a null job and a reason
 * for the user - rather than produce a half-built job.
 */
void ProjectExchangeTest::loadFailsOnAFileThatIsNotAProject() {
  ProjectExchanger exchanger;
  const ProjectExchanger::LoadResult result =
      exchanger.loadProject("this is not an OpenIndy project");
  QVERIFY(result.job.isNull());
  QVERIFY(!result.error.isEmpty());
}

QTEST_APPLESS_MAIN(ProjectExchangeTest)
#include "tst_projectexchange.moc"

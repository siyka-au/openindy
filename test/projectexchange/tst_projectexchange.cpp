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

// every ProjectExchanger::loadProject call dereferences the active
// station/coordinate system unconditionally, and the "create Bundle01_1 if
// not exists" step requires a "Bundle01" system and a "STATION01" station to
// already exist - same minimal setup test/dialogs/dialogstest.cpp uses to
// get a save/load round trip to succeed at all.
QPointer<OiJob> makeJobWithStationAndSystem() {
  QPointer<OiJob> job = new OiJob();

  QPointer<FeatureWrapper> stationFeature = new FeatureWrapper();
  QPointer<Station> station = new Station();
  station->setFeatureName("STATION01");
  stationFeature->setStation(station);
  job->addFeature(stationFeature);

  QPointer<FeatureWrapper> systemFeature = new FeatureWrapper();
  QPointer<CoordinateSystem> system = new CoordinateSystem();
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

QPointer<FeatureWrapper> addPoint(const QPointer<OiJob>& job,
                                  const QString& name, double x, double y,
                                  double z) {
  QPointer<FeatureWrapper> wrapper = new FeatureWrapper();
  QPointer<Point> point = new Point(Position(x, y, z));
  point->setFeatureName(name);
  point->setIsSolved(true);
  wrapper->setPoint(point);
  job->addFeature(wrapper);
  return wrapper;
}

QPointer<FeatureWrapper> addPlane(const QPointer<OiJob>& job,
                                  const QString& name) {
  QPointer<FeatureWrapper> wrapper = new FeatureWrapper();
  QPointer<Plane> plane = new Plane();
  plane->setFeatureName(name);
  wrapper->setPlane(plane);
  job->addFeature(wrapper);
  return wrapper;
}

QPointer<FeatureWrapper> addSphere(const QPointer<OiJob>& job,
                                   const QString& name) {
  QPointer<FeatureWrapper> wrapper = new FeatureWrapper();
  QPointer<Sphere> sphere = new Sphere();
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
  QPointer<OiJob> job = makeJobWithStationAndSystem();

  QPointer<FeatureWrapper> plane = addPlane(job, "Plane01");
  QPointer<FeatureWrapper> sphere = addSphere(job, "Sphere01");

  QPointer<FeatureWrapper> groupWrapper = new FeatureWrapper();
  QPointer<Group> group = new Group();
  group->setFeatureName("MixedGroup");
  groupWrapper->setGroup(group);
  QVERIFY2(group->addMember(plane),
           "plane should be accepted by an unconstrained group");
  QVERIFY2(group->addMember(sphere),
           "sphere should be accepted by an unconstrained group");
  QVERIFY(job->addFeature(groupWrapper));

  QCOMPARE(group->memberCount(), 2);

  ProjectExchanger exchanger;
  QDomDocument xml = exchanger.saveProject(job);

  QPointer<OiJob> reloaded = exchanger.loadProject(xml);
  QVERIFY2(!reloaded.isNull(), "the project should reload without error");

  const QList<QPointer<FeatureWrapper> > reloadedGroups =
      reloaded->getFeaturesByType(FeatureTypes::eGroupFeature);
  QCOMPARE(reloadedGroups.size(), 1);

  QPointer<Group> reloadedGroup = reloadedGroups.first()->getGroup();
  QVERIFY(!reloadedGroup.isNull());
  QCOMPARE(reloadedGroup->getFeatureName(), QString("MixedGroup"));

  // this is the assertion that would have failed before Stage 7e:
  // pendingMemberIds was populated by fromOpenIndyXML and never resolved,
  // so a reloaded group was always empty regardless of what was saved
  QCOMPARE(reloadedGroup->memberCount(), 2);

  QStringList memberNames;
  foreach (const QPointer<FeatureWrapper>& member,
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
  QPointer<OiJob> job = makeJobWithStationAndSystem();

  QPointer<FeatureWrapper> p1 = addPoint(job, "P1", 0.0, 0.0, 0.0);
  QPointer<FeatureWrapper> p2 = addPoint(job, "P2", 1.0, 0.0, 0.0);
  QPointer<FeatureWrapper> p3 = addPoint(job, "P3", 0.0, 1.0, 0.0);

  QPointer<FeatureWrapper> groupWrapper = new FeatureWrapper();
  QPointer<PointGroup> pointGroup = new PointGroup();
  pointGroup->setFeatureName("Wall");
  groupWrapper->setPointGroup(pointGroup);
  QVERIFY(pointGroup->addMember(p1));
  QVERIFY(pointGroup->addMember(p2));
  QVERIFY(pointGroup->addMember(p3));
  QVERIFY(job->addFeature(groupWrapper));

  ProjectExchanger exchanger;
  QDomDocument xml = exchanger.saveProject(job);

  QPointer<OiJob> reloaded = exchanger.loadProject(xml);
  QVERIFY(!reloaded.isNull());

  // must come back as a PointGroup, not a plain Group
  QCOMPARE(reloaded->getFeaturesByType(FeatureTypes::eGroupFeature).size(), 0);
  const QList<QPointer<FeatureWrapper> > reloadedPointGroups =
      reloaded->getFeaturesByType(FeatureTypes::ePointGroupFeature);
  QCOMPARE(reloadedPointGroups.size(), 1);

  QPointer<PointGroup> reloadedGroup =
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
  QPointer<OiJob> job = makeJobWithStationAndSystem();

  QPointer<FeatureWrapper> plane = addPlane(job, "Plane01");
  QPointer<FeatureWrapper> sphere = addSphere(job, "Sphere01");

  QPointer<FeatureWrapper> groupWrapper = new FeatureWrapper();
  QPointer<Group> group = new Group("Planes", FeatureTypes::ePlaneFeature);
  groupWrapper->setGroup(group);
  QVERIFY2(group->addMember(plane),
           "plane should be accepted by a plane-constrained group");
  QVERIFY2(!group->addMember(sphere),
           "sphere should be rejected by a plane-constrained group");
  QVERIFY(job->addFeature(groupWrapper));

  ProjectExchanger exchanger;
  QDomDocument xml = exchanger.saveProject(job);

  QPointer<OiJob> reloaded = exchanger.loadProject(xml);
  QVERIFY(!reloaded.isNull());

  const QList<QPointer<FeatureWrapper> > reloadedGroups =
      reloaded->getFeaturesByType(FeatureTypes::eGroupFeature);
  QCOMPARE(reloadedGroups.size(), 1);

  QPointer<Group> reloadedGroup = reloadedGroups.first()->getGroup();
  QVERIFY(!reloadedGroup.isNull());
  QVERIFY2(reloadedGroup->getConstraint().has_value(),
           "the constraint itself must survive reload");
  QCOMPARE(reloadedGroup->getConstraint().value(), FeatureTypes::ePlaneFeature);
  QCOMPARE(reloadedGroup->memberCount(), 1);

  // find the reloaded sphere by name - a fresh, unconstrained wrapper
  // cannot be built here because accepts() checks the feature's own type,
  // which the reloaded Sphere object still correctly reports
  QPointer<FeatureWrapper> reloadedSphere;
  foreach (const QPointer<FeatureWrapper>& candidate,
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

QTEST_APPLESS_MAIN(ProjectExchangeTest)
#include "tst_projectexchange.moc"

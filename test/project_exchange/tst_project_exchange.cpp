#include <QByteArray>
#include <QPointer>
#include <QtTest>
#include <vector>

#include "choose_la_lib.h"
#include "coordinate_system.h"
#include "feature_util.h"
#include "feature_wrapper.h"
#include "geometry/plane.h"
#include "geometry/point.h"
#include "geometry/sphere.h"
#include "group.h"
#include "oi_job.h"
#include "point_group.h"
#include "project_exchanger.h"
#include "qt_convert.h"
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
 * (tst_point_collection.cpp already covers Group's own membership mechanics).
 */
namespace {

// a project needs an active station and coordinate system, and loading
// creates "Bundle01_1" if it is missing, which requires a "Bundle01" system
// and a "STATION01" station - the same minimal setup
// test/dialogs/dialogs_test.cpp uses to get a save/load round trip to succeed.
std::unique_ptr<OiJob> makeJobWithStationAndSystem() {
  auto ownedJob = std::make_unique<OiJob>();
  const oi::Guarded<OiJob> job = ownedJob.get();

  auto own_station = std::make_unique<oi::Station>();
  oi::Guarded<oi::Station> station = own_station.get();
  oi::Guarded<oi::FeatureWrapper> stationFeature = station->getFeatureWrapper();
  station->setFeatureName("STATION01");
  job->addFeature(std::move(own_station));

  auto own_system = std::make_unique<oi::CoordinateSystem>();
  oi::Guarded<oi::CoordinateSystem> system = own_system.get();
  oi::Guarded<oi::FeatureWrapper> systemFeature = system->getFeatureWrapper();
  system->setFeatureName("PART");
  job->addFeature(std::move(own_system));

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

  return ownedJob;
}

oi::Guarded<FeatureWrapper> addPoint(const oi::Guarded<OiJob>& job,
                                     const QString& name, double x, double y,
                                     double z) {
  auto own_point = std::make_unique<oi::Point>(Position(x, y, z));
  oi::Guarded<oi::Point> point = own_point.get();
  oi::Guarded<oi::FeatureWrapper> wrapper = point->getFeatureWrapper();
  point->setFeatureName(toStd(name));
  point->setIsSolved(true);
  job->addFeature(std::move(own_point));
  return wrapper;
}

oi::Guarded<FeatureWrapper> addPlane(const oi::Guarded<OiJob>& job,
                                     const QString& name) {
  auto own_plane = std::make_unique<oi::Plane>();
  oi::Guarded<oi::Plane> plane = own_plane.get();
  oi::Guarded<oi::FeatureWrapper> wrapper = plane->getFeatureWrapper();
  plane->setFeatureName(toStd(name));
  job->addFeature(std::move(own_plane));
  return wrapper;
}

oi::Guarded<FeatureWrapper> addSphere(const oi::Guarded<OiJob>& job,
                                      const QString& name) {
  auto own_sphere = std::make_unique<oi::Sphere>();
  oi::Guarded<oi::Sphere> sphere = own_sphere.get();
  oi::Guarded<oi::FeatureWrapper> wrapper = sphere->getFeatureWrapper();
  sphere->setFeatureName(toStd(name));
  job->addFeature(std::move(own_sphere));
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
  const auto ownedJob = makeJobWithStationAndSystem();
  const oi::Guarded<OiJob> job = ownedJob.get();

  oi::Guarded<FeatureWrapper> plane = addPlane(job, "Plane01");
  oi::Guarded<FeatureWrapper> sphere = addSphere(job, "Sphere01");

  auto own_group = std::make_unique<oi::Group>();
  oi::Guarded<oi::Group> group = own_group.get();
  oi::Guarded<oi::FeatureWrapper> groupWrapper = group->getFeatureWrapper();
  group->setFeatureName("MixedGroup");
  QVERIFY2(group->addMember(plane),
           "plane should be accepted by an unconstrained group");
  QVERIFY2(group->addMember(sphere),
           "sphere should be accepted by an unconstrained group");
  QVERIFY(job->addFeature(std::move(own_group)));

  QCOMPARE(group->memberCount(), 2);

  ProjectExchanger exchanger;
  const QByteArray xml = exchanger.saveProject(job);

  const auto reloaded = exchanger.loadProject(xml).job;
  QVERIFY2(reloaded != nullptr, "the project should reload without error");

  const std::vector<oi::Guarded<FeatureWrapper> > reloadedGroups =
      reloaded->getFeaturesByType(FeatureTypes::eGroupFeature);
  QCOMPARE(reloadedGroups.size(), 1);

  oi::Guarded<Group> reloadedGroup = reloadedGroups.front()->getGroup();
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
      memberNames << toQString(member->getFeature()->getFeatureName());
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
  const auto ownedJob = makeJobWithStationAndSystem();
  const oi::Guarded<OiJob> job = ownedJob.get();

  oi::Guarded<FeatureWrapper> p1 = addPoint(job, "P1", 0.0, 0.0, 0.0);
  oi::Guarded<FeatureWrapper> p2 = addPoint(job, "P2", 1.0, 0.0, 0.0);
  oi::Guarded<FeatureWrapper> p3 = addPoint(job, "P3", 0.0, 1.0, 0.0);

  auto own_pointGroup = std::make_unique<oi::PointGroup>();
  oi::Guarded<oi::PointGroup> pointGroup = own_pointGroup.get();
  oi::Guarded<oi::FeatureWrapper> groupWrapper =
      pointGroup->getFeatureWrapper();
  pointGroup->setFeatureName("Wall");
  QVERIFY(pointGroup->addMember(p1));
  QVERIFY(pointGroup->addMember(p2));
  QVERIFY(pointGroup->addMember(p3));
  QVERIFY(job->addFeature(std::move(own_pointGroup)));

  ProjectExchanger exchanger;
  const QByteArray xml = exchanger.saveProject(job);

  const auto reloaded = exchanger.loadProject(xml).job;
  QVERIFY(reloaded != nullptr);

  // must come back as a PointGroup, not a plain Group
  QCOMPARE(reloaded->getFeaturesByType(FeatureTypes::eGroupFeature).size(), 0);
  const std::vector<oi::Guarded<FeatureWrapper> > reloadedPointGroups =
      reloaded->getFeaturesByType(FeatureTypes::ePointGroupFeature);
  QCOMPARE(reloadedPointGroups.size(), 1);

  oi::Guarded<PointGroup> reloadedGroup =
      reloadedPointGroups.front()->getPointGroup();
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
  const auto ownedJob = makeJobWithStationAndSystem();
  const oi::Guarded<OiJob> job = ownedJob.get();

  oi::Guarded<FeatureWrapper> plane = addPlane(job, "Plane01");
  oi::Guarded<FeatureWrapper> sphere = addSphere(job, "Sphere01");

  auto own_group =
      std::make_unique<oi::Group>("Planes", FeatureTypes::ePlaneFeature);
  oi::Guarded<oi::Group> group = own_group.get();
  oi::Guarded<oi::FeatureWrapper> groupWrapper = group->getFeatureWrapper();
  QVERIFY2(group->addMember(plane),
           "plane should be accepted by a plane-constrained group");
  QVERIFY2(!group->addMember(sphere),
           "sphere should be rejected by a plane-constrained group");
  QVERIFY(job->addFeature(std::move(own_group)));

  ProjectExchanger exchanger;
  const QByteArray xml = exchanger.saveProject(job);

  const auto reloaded = exchanger.loadProject(xml).job;
  QVERIFY(reloaded != nullptr);

  const std::vector<oi::Guarded<FeatureWrapper> > reloadedGroups =
      reloaded->getFeaturesByType(FeatureTypes::eGroupFeature);
  QCOMPARE(reloadedGroups.size(), 1);

  oi::Guarded<Group> reloadedGroup = reloadedGroups.front()->getGroup();
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
  QVERIFY(!result.job);
  QVERIFY(!result.error.isEmpty());
}

QTEST_APPLESS_MAIN(ProjectExchangeTest)
#include "tst_project_exchange.moc"

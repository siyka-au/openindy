#include <QSignalSpy>
#include <QtTest>

#include "choose_la_lib.h"
#include "oi_job.h"
#include "point.h"
#include "qt_executor.h"
#include "qt_job_adapter.h"
#include "station.h"

using namespace oi;

class QtJobAdapterTest : public QObject {
  Q_OBJECT

 private slots:
  void initTestCase();

  void ofReturnsTheSameAdapterEachTime();
  void nullJobHasNoAdapter();
  void eachJobHasItsOwnAdapter();
  void signalsKeepTheirOldNamesAndArguments();
  void messagesAndRecalculationArriveAsSignals();
  void deferredIsSolvedStaysDeferred();
  void adapterIsCleanedUpWithTheJob();
  void jobDeletedLaterLeavesNothingBehind();
};

void QtJobAdapterTest::initTestCase() {
  ChooseLALib::setLinearAlgebra(ChooseLALib::LinearAlgebraLibrary::Armadillo);
}

void QtJobAdapterTest::ofReturnsTheSameAdapterEachTime() {
  {
    OiJob job;
    QtJobAdapter* first = QtJobAdapter::of(&job);
    QVERIFY(first != nullptr);
    QCOMPARE(QtJobAdapter::of(&job), first);
    QCOMPARE(QtJobAdapter::of(oi::Guarded<OiJob>(&job)), first);
    QCOMPARE(QtJobAdapter::registrySize(), 1);
  }
  QCOMPARE(QtJobAdapter::registrySize(), 0);
}

void QtJobAdapterTest::nullJobHasNoAdapter() {
  QVERIFY(QtJobAdapter::of(static_cast<OiJob*>(nullptr)) == nullptr);
  QVERIFY(QtJobAdapter::of(oi::Guarded<OiJob>()) == nullptr);
  QCOMPARE(QtJobAdapter::registrySize(), 0);
}

void QtJobAdapterTest::eachJobHasItsOwnAdapter() {
  {
    OiJob a, b;
    QVERIFY(QtJobAdapter::of(&a) != QtJobAdapter::of(&b));
    QCOMPARE(QtJobAdapter::registrySize(), 2);
  }
  QCOMPARE(QtJobAdapter::registrySize(), 0);
}

void QtJobAdapterTest::signalsKeepTheirOldNamesAndArguments() {
  OiJob job;
  QtJobAdapter* adapter = QtJobAdapter::of(&job);
  QSignalSpy setSpy(adapter, &QtJobAdapter::featureSetChanged);
  QSignalSpy geomSpy(adapter, &QtJobAdapter::geometrySetChanged);
  QSignalSpy nameSpy(adapter, &QtJobAdapter::featureNameChanged);
  QSignalSpy activeSpy(adapter, &QtJobAdapter::activeFeatureChanged);
  QSignalSpy attrSpy(adapter, &QtJobAdapter::featureAttributesChanged);

  auto own_point = std::make_unique<Point>();
  Point& point = *own_point;
  point.setFeatureName("P1");
  QVERIFY(job.addFeature(std::move(own_point)));
  QCOMPARE(setSpy.count(), 1);
  QCOMPARE(geomSpy.count(), 1);

  point.setFeatureName("renamed");
  QCOMPARE(nameSpy.count(), 1);
  QCOMPARE(nameSpy.first().at(0).toInt(), point.getId());
  QCOMPARE(nameSpy.first().at(1).toString(), QString("P1"));
  QCOMPARE(attrSpy.count(), 1);

  point.setActiveFeatureState(true);
  QCOMPARE(activeSpy.count(), 1);
}

void QtJobAdapterTest::messagesAndRecalculationArriveAsSignals() {
  OiJob job;
  QtJobAdapter* adapter = QtJobAdapter::of(&job);
  QSignalSpy msgSpy(adapter, &QtJobAdapter::sendMessage);
  QSignalSpy featSpy(adapter, &QtJobAdapter::featureRecalculated);
  QSignalSpy allSpy(adapter, &QtJobAdapter::featuresRecalculated);
  QSignalSpy setSpy(adapter, &QtJobAdapter::recalcFeatureSet);

  job.sendMessage("hi", MessageTypes::eInformationMessage);
  QCOMPARE(msgSpy.count(), 1);
  QCOMPARE(msgSpy.first().at(0).toString(), QString("hi"));

  job.featureRecalculated(3);
  job.featuresRecalculated();
  QCOMPARE(featSpy.count(), 1);
  QCOMPARE(featSpy.first().at(0).toInt(), 3);
  QCOMPARE(allSpy.count(), 1);

  auto own_point = std::make_unique<Point>();
  Point* point = own_point.get();
  point->setFeatureName("P");
  QVERIFY(job.addFeature(std::move(own_point)));
  QVERIFY(job.removeFeature(point->getId()));
  QCOMPARE(setSpy.count(), 1);
}

void QtJobAdapterTest::deferredIsSolvedStaysDeferred() {
  QtExecutor executor;
  OiJob job;
  job.setExecutor(&executor);
  QSignalSpy spy(QtJobAdapter::of(&job), &QtJobAdapter::featureIsSolvedChanged);
  auto own_point = std::make_unique<Point>();
  Point& point = *own_point;
  point.setFeatureName("P");
  QVERIFY(job.addFeature(std::move(own_point)));
  point.setIsSolved(true);
  QCOMPARE(spy.count(), 0);
  QCoreApplication::processEvents();
  QCOMPARE(spy.count(), 1);
}

void QtJobAdapterTest::adapterIsCleanedUpWithTheJob() {
  QCOMPARE(QtJobAdapter::registrySize(), 0);
  OiJob* job = new OiJob();
  QPointer<QtJobAdapter> adapter = QtJobAdapter::of(job);
  QVERIFY(!adapter.isNull());
  QCOMPARE(QtJobAdapter::registrySize(), 1);
  delete job;
  QVERIFY(adapter.isNull());
  QCOMPARE(QtJobAdapter::registrySize(), 0);
}

void QtJobAdapterTest::jobDeletedLaterLeavesNothingBehind() {
  // a job with features in it and a connected consumer, deleted through the
  // event loop, must not leave a registry entry or observer behind
  OiJob* job = new OiJob();
  QtJobAdapter* adapter = QtJobAdapter::of(job);
  int notified = 0;
  QObject context;
  QObject::connect(adapter, &QtJobAdapter::featureSetChanged, &context,
                   [&notified] { ++notified; });
  auto own_point = std::make_unique<Point>();
  Point* point = own_point.get();
  point->setFeatureName("P");
  QVERIFY(job->addFeature(std::move(own_point)));
  QCOMPARE(notified, 1);
  delete job;
  QCOMPARE(QtJobAdapter::registrySize(), 0);
}

QTEST_MAIN(QtJobAdapterTest)
#include "tst_qt_job_adapter.moc"

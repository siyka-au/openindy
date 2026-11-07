#include <QSignalSpy>
#include <QThread>
#include <QtTest>
#include <atomic>
#include <map>
#include <thread>
#include <vector>

#include "chooselalib.h"
#include "featurewrapper.h"
#include "oijob.h"
#include "qtexecutor.h"
#include "qtstationadapter.h"
#include "reading.h"
#include "sensor.h"
#include "station.h"

using namespace oi;

Q_DECLARE_METATYPE(oi::SensorAction)
Q_DECLARE_METATYPE(oi::MessageTypes)
Q_DECLARE_METATYPE(oi::MessageDestinations)
Q_DECLARE_METATYPE(std::vector<oi::Guarded<oi::Reading>>)

namespace {

// a driver that is always connected and measures two points
class FakeSensor : public Sensor {
 public:
  std::atomic<int> streamCounter{0};
  bool isSensorAsync() const override { return false; }
  bool getConnectionState() override { return true; }
  bool getIsReadyForMeasurement() override { return true; }
  bool getIsBusy() override { return false; }
  std::map<std::string, std::string> getSensorStatus() override {
    return {{"a", "b"}};
  }
  std::vector<oi::Guarded<Reading>> measure(const MeasurementConfig&) override {
    std::vector<oi::Guarded<Reading>> result;
    for (int i = 0; i < 2; ++i) {
      ReadingCartesian r;
      r.isValid = true;
      result.push_back(new Reading(r));
    }
    return result;
  }
  oi::ValueMap readingStream(const ReadingTypes&) override {
    QThread::msleep(1);
    oi::ValueMap m;
    m["n"] = ++this->streamCounter;
    return m;
  }
  bool accept(const SensorFunctions&, const SensorAttributes&) override {
    return true;
  }
};

}  // namespace

class QtStationAdapterTest : public QObject {
  Q_OBJECT

 private slots:
  void initTestCase();

  void ofReturnsTheSameAdapterEachTime();
  void nullStationHasNoAdapter();
  void adapterIsCleanedUpWithTheStation();
  void resultsArriveAsSignalsOnTheUiThreadInOrder();
  void streamingResultsArriveOnTheUiThread();
  void sensorMessagesAndStatusArriveAsSignals();

  void executorRunsPostedTasksOnItsThreadInOrder();
  void executorDefersTasksPostedFromItsOwnThread();
  void executorDropsPendingTasksWhenItIsDestroyed();
};

void QtStationAdapterTest::initTestCase() {
  ChooseLALib::setLinearAlgebra(ChooseLALib::LinearAlgebraLibrary::Armadillo);
  qRegisterMetaType<oi::SensorAction>("oi::SensorAction");
  qRegisterMetaType<oi::MessageTypes>("oi::MessageTypes");
  qRegisterMetaType<oi::MessageDestinations>("oi::MessageDestinations");
  qRegisterMetaType<std::vector<oi::Guarded<oi::Reading>>>(
      "std::vector<oi::Guarded<oi::Reading>>");
}

void QtStationAdapterTest::ofReturnsTheSameAdapterEachTime() {
  {
    Station station("S1");
    QtStationAdapter* first = QtStationAdapter::of(&station);
    QVERIFY(first != nullptr);
    QCOMPARE(QtStationAdapter::of(&station), first);
    QCOMPARE(QtStationAdapter::of(oi::Guarded<Station>(&station)), first);
    QCOMPARE(QtStationAdapter::registrySize(), 1);
    QCOMPARE(station.getStationObserver(),
             static_cast<StationObserver*>(first));
  }
  QCOMPARE(QtStationAdapter::registrySize(), 0);
}

void QtStationAdapterTest::nullStationHasNoAdapter() {
  QVERIFY(QtStationAdapter::of(static_cast<Station*>(nullptr)) == nullptr);
  QVERIFY(QtStationAdapter::of(oi::Guarded<Station>()) == nullptr);
}

void QtStationAdapterTest::adapterIsCleanedUpWithTheStation() {
  QPointer<QtStationAdapter> adapter;
  {
    Station station("S1");
    adapter = QtStationAdapter::of(&station);
  }
  QVERIFY(adapter.isNull());
  QCOMPARE(QtStationAdapter::registrySize(), 0);
}

void QtStationAdapterTest::resultsArriveAsSignalsOnTheUiThreadInOrder() {
  QtExecutor executor;
  OiJob job;
  job.setExecutor(&executor);
  auto own_station = std::make_unique<oi::Station>("S1");
  oi::Guarded<oi::Station> station = own_station.get();
  oi::Guarded<oi::FeatureWrapper> feature = station->getFeatureWrapper();
  job.addFeature(std::move(own_station));
  station->setActiveStationState(true);

  FakeSensor sensor;
  station->setSensor(&sensor);

  QtStationAdapter* adapter = QtStationAdapter::of(station);
  QStringList order;
  std::vector<QThread*> threads;
  auto note = [&](const QString& what) {
    order << what;
    threads.push_back(QThread::currentThread());
  };
  QObject::connect(adapter, &QtStationAdapter::commandFinished, this,
                   [&](bool ok, QString, SensorAction) {
                     note(QString("finished:%1").arg(ok));
                   });
  QObject::connect(adapter, &QtStationAdapter::measurementFinished, this,
                   [&](int geomId, std::vector<oi::Guarded<Reading>> readings) {
                     note(QString("measurementFinished:%1:%2")
                              .arg(geomId)
                              .arg(readings.size()));
                   });
  QObject::connect(adapter, &QtStationAdapter::measurementDone, this,
                   [&](bool ok) { note(QString("done:%1").arg(ok)); });

  station->connectSensor();  // already connected: reports that
  station->measure(5, MeasurementConfig());

  // nothing is delivered synchronously from the sensor thread
  QVERIFY(order.isEmpty());
  QTRY_COMPARE_WITH_TIMEOUT(order.size(), 4, 5000);
  QCOMPARE(order, (QStringList{"finished:0", "finished:1",
                               "measurementFinished:5:2", "done:1"}));
  for (QThread* t : threads) {
    QCOMPARE(t, QThread::currentThread());
  }
}

void QtStationAdapterTest::streamingResultsArriveOnTheUiThread() {
  QtExecutor executor;
  Station station("S1");
  station.setExecutor(&executor);
  station.setActiveStationState(true);
  FakeSensor sensor;
  station.setSensor(&sensor);

  QtStationAdapter* adapter = QtStationAdapter::of(&station);
  QSignalSpy readings(adapter, &QtStationAdapter::realTimeReading);
  QSignalSpy status(adapter, &QtStationAdapter::realTimeStatus);
  std::vector<QThread*> threads;
  QObject::connect(
      adapter, &QtStationAdapter::realTimeReading, this,
      [&](QVariantMap) { threads.push_back(QThread::currentThread()); });

  station.startReadingStream();
  station.startStatusMonitoringStream();
  QTRY_VERIFY_WITH_TIMEOUT(readings.count() >= 5 && status.count() >= 2, 5000);
  station.stopReadingStream();
  station.stopStatusMonitoringStream();

  for (QThread* t : threads) {
    QCOMPARE(t, QThread::currentThread());
  }
  // in order: 1, 2, 3, ...
  for (int i = 0; i < readings.count(); ++i) {
    QCOMPARE(readings.at(i).at(0).toMap().value("n").toInt(), i + 1);
  }
}

void QtStationAdapterTest::sensorMessagesAndStatusArriveAsSignals() {
  QtExecutor executor;
  Station station("S1");
  station.setExecutor(&executor);
  station.setActiveStationState(true);
  FakeSensor sensor;
  station.setSensor(&sensor);

  QtStationAdapter* adapter = QtStationAdapter::of(&station);
  QSignalSpy messages(adapter, &QtStationAdapter::sensorMessage);
  QSignalSpy statuses(adapter, &QtStationAdapter::sensorStatus);

  // reported by a driver thread
  std::thread driver([&sensor] {
    sensor.sensorMessage("hello", MessageTypes::eWarningMessage);
    sensor.sensorStatus(SensorStatus::eReadyForMeasurement, "ready");
  });
  driver.join();

  QTRY_COMPARE_WITH_TIMEOUT(messages.count(), 1, 5000);
  QCOMPARE(messages.at(0).at(0).toString(), QString("hello"));
  QCOMPARE(messages.at(0).at(1).value<MessageTypes>(),
           MessageTypes::eWarningMessage);
  QCOMPARE(messages.at(0).at(2).value<MessageDestinations>(),
           MessageDestinations::eConsoleMessage);
  QTRY_COMPARE_WITH_TIMEOUT(statuses.count(), 1, 5000);
  QCOMPARE(statuses.at(0).at(0).value<SensorStatus>(),
           SensorStatus::eReadyForMeasurement);
}

void QtStationAdapterTest::executorRunsPostedTasksOnItsThreadInOrder() {
  QtExecutor executor;
  std::vector<int> order;
  std::vector<QThread*> threads;

  std::thread producer([&] {
    for (int i = 0; i < 200; ++i) {
      executor.post([&, i] {
        order.push_back(i);
        threads.push_back(QThread::currentThread());
      });
    }
  });
  producer.join();
  QTRY_COMPARE_WITH_TIMEOUT(static_cast<int>(order.size()), 200, 5000);
  for (int i = 0; i < 200; ++i) {
    QCOMPARE(order[static_cast<std::size_t>(i)], i);
    QCOMPARE(threads[static_cast<std::size_t>(i)], QThread::currentThread());
  }
}

void QtStationAdapterTest::executorDefersTasksPostedFromItsOwnThread() {
  QtExecutor executor;
  bool ran = false;
  executor.post([&ran] { ran = true; });
  QVERIFY(!ran);  // queued, not inline
  QTRY_VERIFY(ran);
}

void QtStationAdapterTest::executorDropsPendingTasksWhenItIsDestroyed() {
  bool ran = false;
  {
    QtExecutor executor;
    executor.post([&ran] { ran = true; });
  }
  QCoreApplication::processEvents();
  QVERIFY(!ran);
}

QTEST_MAIN(QtStationAdapterTest)
#include "tst_qtstationadapter.moc"

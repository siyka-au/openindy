#include <QList>
#include <QPointer>
#include <QString>
#include <QtTest>
#include <vector>

#include "controller_sensor_actions.h"
#include "feature_table_model.h"
#include "measure_behavior_types.h"
#include "qt_convert.h"
#include "test_controller_sensor_actions.h"
#include "test_measure_behavior_logic.h"

using namespace oi;

class MeasureBehaviorTest : public QObject {
  Q_OBJECT

 public:
  MeasureBehaviorTest();

 private Q_SLOTS:
  void testSearch();
  void testSkip();
  void testAimAndMeasure();
};

MeasureBehaviorTest::MeasureBehaviorTest() {}

// basic test
void MeasureBehaviorTest::testAimAndMeasure() {
  TestMeasureBehaviorLogic logic;
  TestControllerSensorActions controller;
  QPointer<FeatureTableModel> featureTableModel;

  std::vector<int> measureFeatures;

  measureFeatures.push_back(101);
  measureFeatures.push_back(102);
  measureFeatures.push_back(103);

  logic.init(&controller, measureFeatures, featureTableModel);
  QCOMPARE(logic.next(), true);
  logic.measure();
  logic.sensorActionFinished(
      true,
      toQString(
          SensorWorkerMessage::MEASUREMENT_FINISHED));  // calls next / measure
  logic.sensorActionFinished(
      true,
      toQString(
          SensorWorkerMessage::MEASUREMENT_FINISHED));  // calls next / measure
  logic.sensorActionFinished(
      true,
      toQString(
          SensorWorkerMessage::MEASUREMENT_FINISHED));  // calls next / measure

  QCOMPARE(logic.next(), false);
  QCOMPARE(logic.featureIds.size(), 3);
  QCOMPARE(controller.actions.join(","),
           QString("startAim,startMeasurement,startAim,startMeasurement,"
                   "startAim,startMeasurement"));
}

void MeasureBehaviorTest::testSkip() {
  TestMeasureBehaviorLogic logic;
  TestControllerSensorActions controller;
  QPointer<FeatureTableModel> featureTableModel;

  std::vector<int> measureFeatures;

  measureFeatures.push_back(101);
  measureFeatures.push_back(102);
  measureFeatures.push_back(103);

  logic.init(&controller, measureFeatures, featureTableModel);
  QCOMPARE(logic.next(), true);
  logic.measure();
  logic.sensorActionFinished(
      true,
      toQString(
          SensorWorkerMessage::MEASUREMENT_FINISHED));  // calls next / measure

  logic.setDecision(Decision::eSkip);
  logic.sensorActionFinished(false,
                             toQString(SensorWorkerMessage::FAILED_TO_MEASURE));

  logic.sensorActionFinished(
      true,
      toQString(
          SensorWorkerMessage::MEASUREMENT_FINISHED));  // calls next / measure

  QCOMPARE(logic.next(), false);
  QCOMPARE(logic.featureIds.size(), 3);
  QCOMPARE(controller.actions.join(","),
           QString("startAim,startMeasurement,startAim,startMeasurement,"
                   "startAim,startMeasurement"));
}

void MeasureBehaviorTest::testSearch() {
  TestMeasureBehaviorLogic logic;
  TestControllerSensorActions controller;
  QPointer<FeatureTableModel> featureTableModel;

  std::vector<int> measureFeatures;

  measureFeatures.push_back(101);
  measureFeatures.push_back(102);
  measureFeatures.push_back(103);

  logic.init(&controller, measureFeatures, featureTableModel);
  QCOMPARE(logic.next(), true);
  logic.measure();
  logic.sensorActionFinished(
      true,
      toQString(
          SensorWorkerMessage::MEASUREMENT_FINISHED));  // calls next / measure

  logic.setDecision(Decision::eSearch);
  logic.sensorActionFinished(false,
                             toQString(SensorWorkerMessage::FAILED_TO_MEASURE));

  logic.sensorActionFinished(
      true,
      toQString(
          SensorWorkerMessage::MEASUREMENT_FINISHED));  // calls next / measure

  QCOMPARE(logic.next(), false);
  QCOMPARE(logic.featureIds.size(), 3);
  QCOMPARE(controller.actions.join(","),
           QString("startAim,startMeasurement,startAim,startMeasurement,"
                   "startSearch,startMeasurement,startAim,startMeasurement"));
}

QTEST_MAIN(
    MeasureBehaviorTest)  // instead of QTEST_APPLESS_MAIN because of: "QWidget:
                          // Must construct a QApplication before a QWidget"

#include "measure_behavior_test.moc"

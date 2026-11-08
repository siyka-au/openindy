#ifndef OI_APP_TEST_MEASURE_BEHAVIOR_TEST_CONTROLLER_SENSOR_ACTIONS_H_
#define OI_APP_TEST_MEASURE_BEHAVIOR_TEST_CONTROLLER_SENSOR_ACTIONS_H_

#include <QtCore>

#include "controller_sensor_actions.h"

class TestControllerSensorActions : public ControllerSensorActions {
 public:
  TestControllerSensorActions();

  // sensor actions
  void startConnect();
  void startDisconnect();
  void startMeasurement();
  void startMove(const oi::Reading& reading);
  void startAim();
  void startAimAndMeasure();
  void startToggleSight();
  void startInitialize();
  void startHome();
  void startCompensation();
  void startChangeMotorState();
  void startCustomAction(const QString& task);
  void startReadingStream(oi::ReadingTypes streamFormat);
  void stopReadingStream();
  void finishMeasurement();
  void startSearch();

  QStringList actions;
};

#endif  // OI_APP_TEST_MEASURE_BEHAVIOR_TEST_CONTROLLER_SENSOR_ACTIONS_H_

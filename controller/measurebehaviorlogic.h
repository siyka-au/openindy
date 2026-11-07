#ifndef MEASUREBEHAVIORLOGIC_H
#define MEASUREBEHAVIORLOGIC_H

#include <QList>
#include <QObject>
#include <QPointer>
#include <vector>

#include "controllersensoractions.h"
#include "featuretablemodel.h"
#include "measurebehaviordialog.h"
#include "measurebehaviortypes.h"
#include "sensorworkermessage.h"

#if defined(OI_MAIN_LIB)
#define OI_MAIN_EXPORT Q_DECL_EXPORT
#else
#define OI_MAIN_EXPORT Q_DECL_IMPORT
#endif

class OI_MAIN_EXPORT MeasureBehaviorLogic : public QObject {
  Q_OBJECT
 public:
  explicit MeasureBehaviorLogic(QObject* parent = nullptr);

  void init(ControllerSensorActions* control, std::vector<int> measureFeatures,
            FeatureTableModel* sourceModel);
  bool next();
  void measure();

 public slots:
  void sensorActionFinished(
      const bool& success, const QString& msg,
      const SensorAction sensorAction = SensorAction::eSensorActionUndefind);
  void setDecision(Decision decision);

 protected slots:
  void handleDecision();

 private slots:
  void abortMeasurement();

 signals:
  void measurementsFinished();
  void closeAllSensorTaskDialogs();

 protected:
  virtual void showCentered(QDialog& dialog);

  virtual void setActiveFeature(FeatureTableModel* sourceModel, int featureId);

 private:
  bool skip();
  bool search();
  bool tryAgain();
  void resetDecision();

  MeasureBehaviorDialog measureBehaviorDialog;

  ControllerSensorActions* control{};
  FeatureTableModel* sourceModel{};
  std::vector<int> measureFeatures;

  int activeFeatureId{-1};
  Decision decision;
};

#endif  // MEASUREBEHAVIORLOGIC_H

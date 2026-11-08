#include "test_measure_behavior_logic.h"

TestMeasureBehaviorLogic::TestMeasureBehaviorLogic() {}

void TestMeasureBehaviorLogic::showCentered(QDialog& dialog) {
  qDebug() << "showCentered";
  this->handleDecision();
}

void TestMeasureBehaviorLogic::setActiveFeature(FeatureTableModel* sourceModel,
                                                int featureId) {
  qDebug() << "setActiveFeature: featureId: " << featureId;
  this->featureIds.push_back(featureId);
}

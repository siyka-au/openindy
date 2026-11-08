#ifndef OI_APP_TEST_MEASURE_BEHAVIOR_TEST_MEASURE_BEHAVIOR_LOGIC_H_
#define OI_APP_TEST_MEASURE_BEHAVIOR_TEST_MEASURE_BEHAVIOR_LOGIC_H_

#include <vector>

#include "measure_behavior_logic.h"

class TestMeasureBehaviorLogic : public MeasureBehaviorLogic {
 public:
  TestMeasureBehaviorLogic();

  std::vector<int> featureIds;

 protected:
  void showCentered(QDialog& dialog);

  void setActiveFeature(FeatureTableModel* sourceModel, int featureId);
};

#endif  // OI_APP_TEST_MEASURE_BEHAVIOR_TEST_MEASURE_BEHAVIOR_LOGIC_H_

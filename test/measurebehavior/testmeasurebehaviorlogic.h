#ifndef TESTMEASUREBEHAVIORLOGIC_H
#define TESTMEASUREBEHAVIORLOGIC_H

#include <vector>

#include "measurebehaviorlogic.h"

class TestMeasureBehaviorLogic : public MeasureBehaviorLogic {
 public:
  TestMeasureBehaviorLogic();

  std::vector<int> featureIds;

 protected:
  void showCentered(QDialog& dialog);

  void setActiveFeature(FeatureTableModel* sourceModel, int featureId);
};

#endif  // TESTMEASUREBEHAVIORLOGIC_H

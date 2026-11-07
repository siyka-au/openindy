#include "featureutil.h"

#include <map>
#include <vector>

// adapted from FeatureUpdater::createBundleTransformations
// create intial bundle transformation (Bundle01_1) to STATION01
bool FeatureUtil::createInitialBundleTransformation(oi::Guarded<OiJob> job) {
  FeatureAttributes trafoAttr;
  trafoAttr.typeOfFeature = FeatureTypes::eTrafoParamFeature;
  trafoAttr.group = "";
  trafoAttr.count = 1;
  trafoAttr.startSystem = "Bundle01";
  trafoAttr.name = QString("%1_1").arg(trafoAttr.startSystem);
  trafoAttr.destinationSystem = "STATION01";

  std::vector<oi::Guarded<FeatureWrapper> > features =
      job->addFeatures(trafoAttr);
  if (features.empty()) {
    return false;
  }
  oi::Guarded<FeatureWrapper> trafoParam = features.front();

  // set up transformation parameters
  if (trafoParam.isNull() || trafoParam->getTrafoParam().isNull()) {
    return false;
  }

  oi::Guarded<TrafoParam> trafo = trafoParam->getTrafoParam();

  // set up parameters
  std::map<TrafoParamParameters, double> parameters;
  parameters.insert_or_assign(TrafoParamParameters::eUnknownTX, 0.0);
  parameters.insert_or_assign(TrafoParamParameters::eUnknownTY, 0.0);
  parameters.insert_or_assign(TrafoParamParameters::eUnknownTZ, 0.0);
  parameters.insert_or_assign(TrafoParamParameters::eUnknownRX, 0.0);
  parameters.insert_or_assign(TrafoParamParameters::eUnknownRY, 0.0);
  parameters.insert_or_assign(TrafoParamParameters::eUnknownRZ, 0.0);
  parameters.insert_or_assign(TrafoParamParameters::eUnknownSX, 1.0);
  parameters.insert_or_assign(TrafoParamParameters::eUnknownSY, 1.0);
  parameters.insert_or_assign(TrafoParamParameters::eUnknownSZ, 1.0);
  trafo->setUnknownParameters(parameters);
  trafo->setIsUsed(true);
  trafo->setIsDatumTrafo(true);

  return true;
}

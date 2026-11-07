#include "featureutil.h"

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

  QList<oi::Guarded<FeatureWrapper> > features = job->addFeatures(trafoAttr);
  if (features.isEmpty()) {
    return false;
  }
  oi::Guarded<FeatureWrapper> trafoParam = features.first();

  // set up transformation parameters
  if (trafoParam.isNull() || trafoParam->getTrafoParam().isNull()) {
    return false;
  }

  oi::Guarded<TrafoParam> trafo = trafoParam->getTrafoParam();

  // set up parameters
  QMap<TrafoParamParameters, double> parameters;
  parameters.insert(TrafoParamParameters::eUnknownTX, 0.0);
  parameters.insert(TrafoParamParameters::eUnknownTY, 0.0);
  parameters.insert(TrafoParamParameters::eUnknownTZ, 0.0);
  parameters.insert(TrafoParamParameters::eUnknownRX, 0.0);
  parameters.insert(TrafoParamParameters::eUnknownRY, 0.0);
  parameters.insert(TrafoParamParameters::eUnknownRZ, 0.0);
  parameters.insert(TrafoParamParameters::eUnknownSX, 1.0);
  parameters.insert(TrafoParamParameters::eUnknownSY, 1.0);
  parameters.insert(TrafoParamParameters::eUnknownSZ, 1.0);
  trafo->setUnknownParameters(parameters);
  trafo->setIsUsed(true);
  trafo->setIsDatumTrafo(true);

  return true;
}

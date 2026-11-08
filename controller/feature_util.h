#ifndef FEATUREUTIL_H
#define FEATUREUTIL_H

#include <QList>
#include <QPointer>
#include <QString>

#include "oi_job.h"

using namespace oi;

class FeatureUtil {
 public:
  // adapted from FeatureUpdater::createBundleTransformations
  // create intial bundle transformation (Bundle01_1) to STATION01
  bool createInitialBundleTransformation(oi::Guarded<OiJob> job);
};

#endif  // FEATUREUTIL_H

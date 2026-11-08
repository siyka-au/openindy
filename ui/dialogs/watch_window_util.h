#ifndef OI_APP_UI_DIALOGS_WATCH_WINDOW_UTIL_H_
#define OI_APP_UI_DIALOGS_WATCH_WINDOW_UTIL_H_

#include <QtCore>

#include "feature.h"
#include "feature_wrapper.h"
#include "geometry.h"
#include "oi_vec.h"
#include "position.h"
#include "radius.h"

#if defined(OI_MAIN_LIB)
#define OI_MAIN_EXPORT Q_DECL_EXPORT
#else
#define OI_MAIN_EXPORT Q_DECL_IMPORT
#endif

namespace oi {

struct Result {
  Position position;
  Radius radius;
  OiVec delta;
  double d3D{};
};

class OI_MAIN_EXPORT WatchWindowUtil {
 public:
  WatchWindowUtil();

  Result getPosition(oi::Guarded<FeatureWrapper> feature, OiVec trackerXYZ);
};
}  // namespace oi
#endif  // OI_APP_UI_DIALOGS_WATCH_WINDOW_UTIL_H_

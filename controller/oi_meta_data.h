#ifndef OI_APP_CONTROLLER_OI_META_DATA_H_
#define OI_APP_CONTROLLER_OI_META_DATA_H_

#include <QJsonArray>
#include <QString>

#include "bundle_adjustment.h"
#include "construct_function.h"
#include "exchange_defined_format.h"
#include "exchange_interface.h"
#include "exchange_simple_ascii.h"
#include "fit_function.h"
#include "generate_feature_function.h"
#include "geodetic_function.h"
#include "laser_tracker.h"
#include "object_transformation.h"
#include "plugin.h"
#include "sensor.h"
#include "special_function.h"
#include "system_transformation.h"
#include "tool.h"
#include "total_station.h"
#include "types.h"

#if defined(OI_MAIN_LIB)
#define OI_MAIN_EXPORT Q_DECL_EXPORT
#else
#define OI_MAIN_EXPORT Q_DECL_IMPORT
#endif

/*!
 * \brief The OiMetaData class
 * Holds version information about OpenIndy
 */
class OI_MAIN_EXPORT OiMetaData {
 public:
  // #########################
  // plugin interface versions
  // #########################

  static QString iid_Plugin;
  static QString iid_LaserTracker;
  static QString iid_TotalStation;
  static QString iid_ConstructFunction;
  static QString iid_FitFunction;
  static QString iid_SpecialFunction;
  static QString iid_GeodeticFunction;
  static QString iid_BundleAdjustment;
  static QString iid_ObjectTransformation;
  static QString iid_SystemTransformation;
  static QString iid_Sensor;
  static QString iid_Simulation;
  static QString iid_Tool;
  static QString iid_Exchange;
  static QString iid_ExchangeSimpleAscii;
  static QString iid_ExchangeDefinedFormat;

  static bool findIID(const QString& iid);

  static SensorType getSensorType(const QString& iid);

  // ###############################
  // get plugin interface type names
  // ###############################

  static QStringList getSensorTypeNames();
  static QString getSensorTypeName(const QString& iid);

  static QStringList getFunctionTypeNames();
  static QString getFunctionTypeName(const QString& iid);

  static QStringList getExchangeTypeNames();
  static QString getExchangeTypeName(const QString& iid);

  // #################################
  // version of the main program (src)
  // #################################

  static QString version;

  // #################################
  // meta data about this installation
  // #################################

  static QString getCompiler();
  static QString getOperatingSys();
};

#endif  // OI_APP_CONTROLLER_OI_META_DATA_H_

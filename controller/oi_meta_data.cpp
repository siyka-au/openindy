#include "oi_meta_data.h"

#include "qt_convert.h"
#include "toolchain.h"

// define version of the main program (src)
QString OiMetaData::version = "0.0.1";

// define plugin interface versions
QString OiMetaData::iid_Plugin = toQString(Plugin_iidd);
QString OiMetaData::iid_Sensor = toQString(Sensor_iidd);
QString OiMetaData::iid_LaserTracker = toQString(LaserTracker_iidd);
QString OiMetaData::iid_TotalStation = toQString(TotalStation_iidd);
QString OiMetaData::iid_ConstructFunction = toQString(ConstructFunction_iidd);
QString OiMetaData::iid_FitFunction = toQString(FitFunction_iidd);
QString OiMetaData::iid_SpecialFunction = toQString(SpecialFunction_iidd);
QString OiMetaData::iid_GeodeticFunction = toQString(GeodeticFunction_iidd);
QString OiMetaData::iid_BundleAdjustment = toQString(BundleAdjustment_iidd);
QString OiMetaData::iid_ObjectTransformation =
    toQString(ObjectTransformation_iidd);
QString OiMetaData::iid_SystemTransformation =
    toQString(SystemTransformation_iidd);
QString OiMetaData::iid_Simulation = toQString(Simulation_iidd);
QString OiMetaData::iid_Tool = toQString(Tool_iidd);
QString OiMetaData::iid_Exchange = toQString(Exchange_iidd);
QString OiMetaData::iid_ExchangeSimpleAscii =
    toQString(ExchangeSimpleAscii_iidd);
QString OiMetaData::iid_ExchangeDefinedFormat =
    toQString(ExchangeDefinedFormat_iidd);

/*!
 * \brief OiMetaData::findIID
 * \param iid
 * \return
 */
bool OiMetaData::findIID(const QString& iid) {
  if (iid.compare(OiMetaData::iid_LaserTracker) == 0) {
    return true;
  } else if (iid.compare(OiMetaData::iid_ConstructFunction) == 0) {
    return true;
  } else if (iid.compare(OiMetaData::iid_FitFunction) == 0) {
    return true;
  } else if (iid.compare(OiMetaData::iid_SpecialFunction) == 0) {
    return true;
  } else if (iid.compare(OiMetaData::iid_GeodeticFunction) == 0) {
    return true;
  } else if (iid.compare(OiMetaData::iid_BundleAdjustment) == 0) {
    return true;
  } else if (iid.compare(OiMetaData::iid_ObjectTransformation) == 0) {
    return true;
  } else if (iid.compare(OiMetaData::iid_SystemTransformation) == 0) {
    return true;
  } else if (iid.compare(OiMetaData::iid_TotalStation) == 0) {
    return true;
  } else if (iid.compare(OiMetaData::iid_Plugin) == 0) {
    return true;
  } else if (iid.compare(OiMetaData::iid_Simulation) == 0) {
    return true;
  } else if (iid.compare(OiMetaData::iid_Tool) == 0) {
    return true;
  } else if (iid.compare(OiMetaData::iid_Exchange) == 0) {
    return true;
  } else if (iid.compare(OiMetaData::iid_ExchangeSimpleAscii) == 0) {
    return true;
  } else if (iid.compare(OiMetaData::iid_ExchangeDefinedFormat) == 0) {
    return true;
  }

  return false;
}

/*!
 * \brief OiMetaData::getSensorType
 * \param iid
 * \return
 */
SensorType OiMetaData::getSensorType(const QString& iid) {
  // derived from the interface id itself, so a plugin can introduce a sensor
  // type the core has never heard of without a core change
  return SensorType::fromInterfaceId(toStd(iid));
}

/*!
 * \brief OiMetaData::getSensorTypeNames
 * \return
 */
QStringList OiMetaData::getSensorTypeNames() {
  QStringList result;

  result.append(OiMetaData::iid_LaserTracker);
  result.append(OiMetaData::iid_TotalStation);

  return result;
}

/*!
 * \brief OiMetaData::getSensorTypeName
 * \param iid
 * \return
 */
QString OiMetaData::getSensorTypeName(const QString& iid) {
  if (iid.compare(OiMetaData::iid_LaserTracker) == 0) {
    return "laser tracker";
  } else if (iid.compare(OiMetaData::iid_TotalStation) == 0) {
    return "total station";
  }
  return "";
}

/*!
 * \brief OiMetaData::getFunctionTypeNames
 * \return
 */
QStringList OiMetaData::getFunctionTypeNames() {
  QStringList result;

  result.append(OiMetaData::iid_FitFunction);
  result.append(OiMetaData::iid_ConstructFunction);
  result.append(OiMetaData::iid_SpecialFunction);
  result.append(OiMetaData::iid_GeodeticFunction);
  result.append(OiMetaData::iid_ObjectTransformation);
  result.append(OiMetaData::iid_SystemTransformation);

  return result;
}

/*!
 * \brief OiMetaData::getFunctionTypeName
 * \param iid
 * \return
 */
QString OiMetaData::getFunctionTypeName(const QString& iid) {
  if (iid.compare(OiMetaData::iid_FitFunction) == 0) {
    return "fit function";
  } else if (iid.compare(OiMetaData::iid_ConstructFunction) == 0) {
    return "construct function";
  } else if (iid.compare(OiMetaData::iid_SpecialFunction) == 0) {
    return "special function";
  } else if (iid.compare(OiMetaData::iid_GeodeticFunction) == 0) {
    return "geodetic function";
  } else if (iid.compare(OiMetaData::iid_ObjectTransformation) == 0) {
    return "object transformation";
  } else if (iid.compare(OiMetaData::iid_SystemTransformation) == 0) {
    return "system transformation";
  }
  return "";
}

/*!
 * \brief OiMetaData::getExchangeTypeNames
 * \return
 */
QStringList OiMetaData::getExchangeTypeNames() {
  QStringList result;

  result.append(OiMetaData::iid_ExchangeSimpleAscii);
  result.append(OiMetaData::iid_ExchangeDefinedFormat);

  return result;
}

/*!
 * \brief OiMetaData::getExchangeTypeName
 * \param iid
 * \return
 */
QString OiMetaData::getExchangeTypeName(const QString& iid) {
  if (iid.compare(OiMetaData::iid_ExchangeSimpleAscii) == 0) {
    return "exchange simple ascii";
  } else if (iid.compare(OiMetaData::iid_ExchangeDefinedFormat) == 0) {
    return "exchange defined format";
  }
  return "";
}

/*!
 * \brief The compiler family this build was made with: "gcc", "clang",
 * "mingw" or "msvc" (core's oi::kToolchainCompiler). Plugins write the same
 * words into the "compiler" field of their metadata; informational only, the
 * loader compares the toolchain string (plugin/toolchain.h).
 */
QString OiMetaData::getCompiler() { return toQString(oi::kToolchainCompiler); }

/*!
 * \brief OiMetaData::getOperatingSys
 * \return
 */
QString OiMetaData::getOperatingSys() {
  QString o;

#ifdef Q_OS_MAC
  o = "mac os";
#endif

#ifdef Q_OS_WIN
  o = "windows";
#endif

#ifdef Q_OS_LINUX
  o = "linux";
#endif

  return o;
}

#include "parameterdisplayconfig.h"

/*!
 * \brief ParameterDisplayConfig::ParameterDisplayConfig
 */
ParameterDisplayConfig::ParameterDisplayConfig() {
  // load default parameter display config
  this->init();
}

/*!
 * \brief ParameterDisplayConfig::getDisplayUnit
 * \param dimension
 * \return
 */
UnitType ParameterDisplayConfig::getDisplayUnit(
    const DimensionType& dimension) const {
  switch (dimension) {
    case DimensionType::eMetric:
      return this->metricDisplayUnit;
    case DimensionType::eAngular:
      return this->angularDisplayUnit;
    case DimensionType::eTemperature:
      return this->temperatureDisplayUnit;
    case DimensionType::eDimensionless:
      return UnitType::eNoUnit;
  }
}

/*!
 * \brief ParameterDisplayConfig::setDisplayUnitType
 * \param dimension
 * \param unit
 */
void ParameterDisplayConfig::setDisplayUnitType(const DimensionType& dimension,
                                                const UnitType& unit) {
  switch (dimension) {
    case DimensionType::eMetric:
      if (unit == UnitType::eUnitMeter || unit == UnitType::eUnitMilliMeter ||
          unit == UnitType::eUnitInch) {
        this->metricDisplayUnit = unit;
      }
      break;
    case DimensionType::eAngular:
      if (unit == UnitType::eUnitRadiant ||
          unit == UnitType::eUnitMilliRadians || unit == UnitType::eUnitGon ||
          unit == UnitType::eUnitDecimalDegree ||
          unit == UnitType::eUnitArcSeconds) {
        this->angularDisplayUnit = unit;
      }
      break;
    case DimensionType::eTemperature:
      if (unit == UnitType::eUnitGrad || unit == UnitType::eUnitKelvin ||
          unit == UnitType::eUnitFahrenheit) {
        this->temperatureDisplayUnit = unit;
      }
      break;
    case DimensionType::eDimensionless:
      break;
  }
}

/*!
 * \brief ParameterDisplayConfig::getDisplayDigits
 * \param dimension
 * \return
 */
int ParameterDisplayConfig::getDisplayDigits(
    const DimensionType& dimension) const {
  switch (dimension) {
    case DimensionType::eMetric:
      return this->metricDisplayDigits;
    case DimensionType::eAngular:
      return this->angularDisplayDigits;
    case DimensionType::eTemperature:
      return this->temperatureDisplayDigits;
    case DimensionType::eDimensionless:
      return this->dimensionlessDisplayDigits;
  }
}

/*!
 * \brief ParameterDisplayConfig::setDisplayDigits
 * \param dimension
 * \param digits
 */
void ParameterDisplayConfig::setDisplayDigits(const DimensionType& dimension,
                                              const int& digits) {
  // check digits
  if (digits < 0) {
    return;
  }

  // set digits
  switch (dimension) {
    case DimensionType::eMetric:
      this->metricDisplayDigits = digits;
      break;
    case DimensionType::eAngular:
      this->angularDisplayDigits = digits;
      break;
    case DimensionType::eTemperature:
      this->temperatureDisplayDigits = digits;
      break;
    case DimensionType::eDimensionless:
      this->dimensionlessDisplayDigits = digits;
      break;
  }
}

/*!
 * \brief ParameterDisplayConfig::getDisplayUnits
 * \return
 */
QMap<DimensionType, UnitType> ParameterDisplayConfig::getDisplayUnits() const {
  QMap<DimensionType, UnitType> displayUnits;

  displayUnits.insert(DimensionType::eMetric, this->metricDisplayUnit);
  displayUnits.insert(DimensionType::eAngular, this->angularDisplayUnit);
  displayUnits.insert(DimensionType::eTemperature,
                      this->temperatureDisplayUnit);

  return displayUnits;
}

/*!
 * \brief ParameterDisplayConfig::getDisplayDigits
 * \return
 */
QMap<DimensionType, int> ParameterDisplayConfig::getDisplayDigits() const {
  QMap<DimensionType, int> displayDigits;

  displayDigits.insert(DimensionType::eMetric, this->metricDisplayDigits);
  displayDigits.insert(DimensionType::eAngular, this->angularDisplayDigits);
  displayDigits.insert(DimensionType::eTemperature,
                       this->temperatureDisplayDigits);
  displayDigits.insert(DimensionType::eDimensionless,
                       this->dimensionlessDisplayDigits);

  return displayDigits;
}

/*!
 * \brief ParameterDisplayConfig::getUseSounds
 * \return
 */
bool ParameterDisplayConfig::getUseSounds() const { return this->useSounds; }

/*!
 * \brief ParameterDisplayConfig::setUseSounds
 * \param use
 */
void ParameterDisplayConfig::setUseSounds(const bool use) {
  this->useSounds = use;
}

/*!
 * \brief ParameterDisplayConfig::init
 * Set up default parameter display config
 */
void ParameterDisplayConfig::init() {
  // default units
  this->metricDisplayUnit = UnitType::eUnitMilliMeter;
  this->angularDisplayUnit = UnitType::eUnitDecimalDegree;
  this->temperatureDisplayUnit = UnitType::eUnitGrad;

  // default digits
  this->metricDisplayDigits = 2;
  this->angularDisplayDigits = 6;
  this->temperatureDisplayDigits = 2;
  this->dimensionlessDisplayDigits = 6;

  this->useSounds = true;
}

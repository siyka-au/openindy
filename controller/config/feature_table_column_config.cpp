#include "feature_table_column_config.h"

#include <utility>
#include <vector>

#include "containers.h"

/*!
 * \brief FeatureTableColumnConfig::FeatureTableColumnConfig
 */
FeatureTableColumnConfig::FeatureTableColumnConfig() {
  // load default column config
  this->init();
}
FeatureTableColumnConfig::FeatureTableColumnConfig(
    std::vector<FeatureDisplayAttributes> displayColumns)
    : displayColumns(displayColumns) {}

/*!
 * \brief FeatureTableColumnConfig::getColumnVisibility
 * \param column
 * \return
 */
bool FeatureTableColumnConfig::getColumnVisibility(
    const FeatureDisplayAttributes& column) const {
  return oi::contains(this->displayColumns, column);
}

/*!
 * \brief FeatureTableColumnConfig::setColumnVisibility
 * \param column
 * \param isVisible
 */
void FeatureTableColumnConfig::setColumnVisibility(
    const FeatureDisplayAttributes& column, const bool& isVisible) {
  if (isVisible && !oi::contains(this->displayColumns, column)) {
    this->displayColumns.push_back(column);
  } else {
    oi::removeOne(this->displayColumns, column);
  }
}

/*!
 * \brief FeatureTableColumnConfig::getColumnPosition
 * Returns the position index of the given column or -1 if the column is not
 * displayed
 * \param column
 * \return
 */
int FeatureTableColumnConfig::getColumnPosition(
    const FeatureDisplayAttributes& column) const {
  return oi::indexOf(this->displayColumns, column);
}

/*!
 * \brief FeatureTableColumnConfig::setColumnPosition
 * \param column
 * \param index
 */
void FeatureTableColumnConfig::setColumnPosition(
    const FeatureDisplayAttributes& column, const int& index) {
  if (oi::contains(this->displayColumns, column) && index >= 0 &&
      std::cmp_less(index, this->displayColumns.size())) {
    oi::removeOne(this->displayColumns, column);
    oi::insertAt(this->displayColumns, index, column);
  }
}

/*!
 * \brief FeatureTableColumnConfig::getDisplayAttributeAt
 * \param column
 * \return
 */
FeatureDisplayAttributes FeatureTableColumnConfig::getDisplayAttributeAt(
    const int& column) const {
  if (std::cmp_greater(this->displayColumns.size(), column)) {
    return this->displayColumns.at(column);
  }
  return eFeatureDisplayType;
}

/*!
 * \brief FeatureTableColumnConfig::init
 * Set up default column config
 */
void FeatureTableColumnConfig::init() {
  // default columns in default order
  this->displayColumns.push_back(eFeatureDisplayType);
  this->displayColumns.push_back(eFeatureDisplayIsActual);
  this->displayColumns.push_back(eFeatureDisplayGroup);
  this->displayColumns.push_back(eFeatureDisplayName);
  this->displayColumns.push_back(eFeatureDisplayX);
  this->displayColumns.push_back(eFeatureDisplayY);
  this->displayColumns.push_back(eFeatureDisplayZ);
  this->displayColumns.push_back(eFeatureDisplayStDev);
  this->displayColumns.push_back(eFeatureDisplayObservations);
  this->displayColumns.push_back(eFeatureDisplayMeasurementConfig);
  this->displayColumns.push_back(eFeatureDisplayFunctions);
  this->displayColumns.push_back(eFeatureDisplayIsCommon);
  this->displayColumns.push_back(eFeatureDisplayPrimaryI);
  this->displayColumns.push_back(eFeatureDisplayPrimaryJ);
  this->displayColumns.push_back(eFeatureDisplayPrimaryK);
  this->displayColumns.push_back(eFeatureDisplayRadiusA);
  this->displayColumns.push_back(eFeatureDisplayIsSolved);
  this->displayColumns.push_back(eFeatureDisplayComment);
  this->displayColumns.push_back(eFeatureDisplayAngle);
  this->displayColumns.push_back(eFeatureDisplayDistance);
  this->displayColumns.push_back(eFeatureDisplayFormError);
}

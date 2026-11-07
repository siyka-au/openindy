#include "trafoparamtablecolumnconfig.h"

#include "containers.h"

/*!
 * \brief TrafoParamTableColumnConfig::FeatureTableColumnConfig
 */
TrafoParamTableColumnConfig::TrafoParamTableColumnConfig(bool initBundle) {
  if (initBundle) {
    this->initBundle();
  } else {
    // load default column config
    this->init();
  }
}

/*!
 * \brief TrafoParamTableColumnConfig::getColumnVisibility
 * \param column
 * \return
 */
bool TrafoParamTableColumnConfig::getColumnVisibility(
    const TrafoParamDisplayAttributes& column) const {
  return oi::contains(this->displayColumns, column);
}

/*!
 * \brief TrafoParamTableColumnConfig::setColumnVisibility
 * \param column
 * \param isVisible
 */
void TrafoParamTableColumnConfig::setColumnVisibility(
    const TrafoParamDisplayAttributes& column, const bool& isVisible) {
  if (isVisible && !oi::contains(this->displayColumns, column)) {
    this->displayColumns.push_back(column);
  } else {
    oi::removeOne(this->displayColumns, column);
  }
}

/*!
 * \brief TrafoParamTableColumnConfig::getColumnPosition
 * Returns the position index of the given column or -1 if the column is not
 * displayed
 * \param column
 * \return
 */
int TrafoParamTableColumnConfig::getColumnPosition(
    const TrafoParamDisplayAttributes& column) const {
  return oi::indexOf(this->displayColumns, column);
}

/*!
 * \brief TrafoParamTableColumnConfig::setColumnPosition
 * \param column
 * \param index
 */
void TrafoParamTableColumnConfig::setColumnPosition(
    const TrafoParamDisplayAttributes& column, const int& index) {
  if (oi::contains(this->displayColumns, column) && index >= 0 &&
      index < this->displayColumns.size()) {
    oi::removeOne(this->displayColumns, column);
    oi::insertAt(this->displayColumns, index, column);
  }
}

/*!
 * \brief TrafoParamTableColumnConfig::getDisplayAttributeAt
 * \param column
 * \return
 */
TrafoParamDisplayAttributes TrafoParamTableColumnConfig::getDisplayAttributeAt(
    const int& column) const {
  if (this->displayColumns.size() > column) {
    return this->displayColumns.at(column);
  }
  return eTrafoParamDisplayType;
}

/*!
 * \brief TrafoParamTableColumnConfig::init
 * Set up default column config
 */
void TrafoParamTableColumnConfig::init() {
  // default columns in default order
  this->displayColumns.push_back(eTrafoParamDisplayName);
  this->displayColumns.push_back(eTrafoParamDisplayGroup);
  this->displayColumns.push_back(eTrafoParamDisplayIsSolved);
  this->displayColumns.push_back(eTrafoParamDisplayIsUsed);
  this->displayColumns.push_back(eTrafoParamDisplayStartSystem);
  this->displayColumns.push_back(eTrafoParamDisplayDestinationSystem);
  this->displayColumns.push_back(eTrafoParamDisplayTranslationX);
  this->displayColumns.push_back(eTrafoParamDisplayTranslationY);
  this->displayColumns.push_back(eTrafoParamDisplayTranslationZ);
  this->displayColumns.push_back(eTrafoParamDisplayRotationX);
  this->displayColumns.push_back(eTrafoParamDisplayRotationY);
  this->displayColumns.push_back(eTrafoParamDisplayRotationZ);
  this->displayColumns.push_back(eTrafoParamDisplayScaleX);
  this->displayColumns.push_back(eTrafoParamDisplayScaleY);
  this->displayColumns.push_back(eTrafoParamDisplayScaleZ);
  this->displayColumns.push_back(eTrafoParamDisplayStDev);
  this->displayColumns.push_back(eTrafoParamDisplayFunctions);
  this->displayColumns.push_back(eTrafoParamDisplayComment);
}

void TrafoParamTableColumnConfig::initBundle() {
  // default columns in default order
  this->displayColumns.push_back(eTrafoParamDisplayName);
  this->displayColumns.push_back(eTrafoParamDisplayIsSolved);
  this->displayColumns.push_back(eTrafoParamDisplayStartSystem);
  this->displayColumns.push_back(eTrafoParamDisplayDestinationSystem);
  this->displayColumns.push_back(eTrafoParamDisplayTranslationX);
  this->displayColumns.push_back(eTrafoParamDisplayTranslationY);
  this->displayColumns.push_back(eTrafoParamDisplayTranslationZ);
  this->displayColumns.push_back(eTrafoParamDisplayRotationX);
  this->displayColumns.push_back(eTrafoParamDisplayRotationY);
  this->displayColumns.push_back(eTrafoParamDisplayRotationZ);
  this->displayColumns.push_back(eTrafoParamDisplayScaleX);
  this->displayColumns.push_back(eTrafoParamDisplayScaleY);
  this->displayColumns.push_back(eTrafoParamDisplayScaleZ);
}

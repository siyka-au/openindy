#include "reading_table_column_config.h"

#include <utility>

#include "containers.h"

/*!
 * \brief ReadingTableColumnConfig::ReadingTableColumnConfig
 */
ReadingTableColumnConfig::ReadingTableColumnConfig() {
  // load default column config
  this->init();
}

/*!
 * \brief ReadingTableColumnConfig::getColumnVisibility
 * \param column
 * \return
 */
bool ReadingTableColumnConfig::getColumnVisibility(
    const ReadingDisplayAttributes& column) const {
  return oi::contains(this->displayColumns, column);
}

/*!
 * \brief ReadingTableColumnConfig::setColumnVisibility
 * \param column
 * \param isVisible
 */
void ReadingTableColumnConfig::setColumnVisibility(
    const ReadingDisplayAttributes& column, const bool& isVisible) {
  if (isVisible && !oi::contains(this->displayColumns, column)) {
    this->displayColumns.push_back(column);
  } else {
    oi::removeOne(this->displayColumns, column);
  }
}

/*!
 * \brief ReadingTableColumnConfig::getColumnPosition
 * Returns the position index of the given column or -1 if the column is not
 * displayed
 * \param column
 * \return
 */
int ReadingTableColumnConfig::getColumnPosition(
    const ReadingDisplayAttributes& column) const {
  return oi::indexOf(this->displayColumns, column);
}

/*!
 * \brief ReadingTableColumnConfig::setColumnPosition
 * \param column
 * \param index
 */
void ReadingTableColumnConfig::setColumnPosition(
    const ReadingDisplayAttributes& column, const int& index) {
  if (oi::contains(this->displayColumns, column) && index >= 0 &&
      std::cmp_less(index, this->displayColumns.size())) {
    oi::removeOne(this->displayColumns, column);
    oi::insertAt(this->displayColumns, index, column);
  }
}

/*!
 * \brief ReadingTableColumnConfig::init
 * Set up default column config
 */
void ReadingTableColumnConfig::init() {
  // default columns in default order
  this->displayColumns.push_back(eReadingDisplayId);
  this->displayColumns.push_back(eReadingDisplayType);
  this->displayColumns.push_back(eReadingDisplayTime);
  this->displayColumns.push_back(eReadingDisplaySensorConfigName);
  this->displayColumns.push_back(eReadingDisplayMeasurementConfigName);
  this->displayColumns.push_back(eReadingDisplaySide);
  this->displayColumns.push_back(eReadingDisplayAzimuth);
  this->displayColumns.push_back(eReadingDisplayZenith);
  this->displayColumns.push_back(eReadingDisplayDistance);
  this->displayColumns.push_back(eReadingDisplayX);
  this->displayColumns.push_back(eReadingDisplayY);
  this->displayColumns.push_back(eReadingDisplayZ);
  this->displayColumns.push_back(eReadingDisplayI);
  this->displayColumns.push_back(eReadingDisplayJ);
  this->displayColumns.push_back(eReadingDisplayK);
  this->displayColumns.push_back(eReadingDisplayTemperature);
  this->displayColumns.push_back(eReadingDisplayImported);
  /*this->displayColumns.append(eReadingDisplaySigmaAzimuth);
  this->displayColumns.append(eReadingDisplaySigmaZenith);
  this->displayColumns.append(eReadingDisplaySigmaDistance);
  this->displayColumns.append(eReadingDisplaySigmaX);
  this->displayColumns.append(eReadingDisplaySigmaY);
  this->displayColumns.append(eReadingDisplaySigmaZ);
  this->displayColumns.append(eReadingDisplaySigmaI);
  this->displayColumns.append(eReadingDisplaySigmaJ);
  this->displayColumns.append(eReadingDisplaySigmaK);
  this->displayColumns.append(eReadingDisplaySigmaTemperature);*/
}

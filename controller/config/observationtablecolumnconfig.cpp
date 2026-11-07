#include "observationtablecolumnconfig.h"

#include <utility>

#include "containers.h"

/*!
 * \brief ObservationTableColumnConfig::ObservationTableColumnConfig
 */
ObservationTableColumnConfig::ObservationTableColumnConfig() {
  // load default column config
  this->init();
}

/*!
 * \brief ObservationTableColumnConfig::getColumnVisibility
 * \param column
 * \return
 */
bool ObservationTableColumnConfig::getColumnVisibility(
    const ObservationDisplayAttributes& column) const {
  return oi::contains(this->displayColumns, column);
}

/*!
 * \brief ObservationTableColumnConfig::setColumnVisibility
 * \param column
 * \param isVisible
 */
void ObservationTableColumnConfig::setColumnVisibility(
    const ObservationDisplayAttributes& column, const bool& isVisible) {
  if (isVisible && !oi::contains(this->displayColumns, column)) {
    this->displayColumns.push_back(column);
  } else {
    oi::removeOne(this->displayColumns, column);
  }
}

/*!
 * \brief ObservationTableColumnConfig::getColumnPosition
 * Returns the position index of the given column or -1 if the column is not
 * displayed
 * \param column
 * \return
 */
int ObservationTableColumnConfig::getColumnPosition(
    const ObservationDisplayAttributes& column) const {
  return oi::indexOf(this->displayColumns, column);
}

/*!
 * \brief ObservationTableColumnConfig::setColumnPosition
 * \param column
 * \param index
 */
void ObservationTableColumnConfig::setColumnPosition(
    const ObservationDisplayAttributes& column, const int& index) {
  if (oi::contains(this->displayColumns, column) && index >= 0 &&
      std::cmp_less(index, this->displayColumns.size())) {
    oi::removeOne(this->displayColumns, column);
    oi::insertAt(this->displayColumns, index, column);
  }
}

/*!
 * \brief ObservationTableColumnConfig::init
 * defines visible (!) columns but not column order (!), ordinal defines column
 * order
 */
void ObservationTableColumnConfig::init() {
  // defines visible (!) columns but not column order (!), ordinal defines
  // column order
  this->displayColumns.push_back(eObservationDisplayId);
  this->displayColumns.push_back(eObservationDisplayStation);
  this->displayColumns.push_back(eObservationDisplayTargetGeometries);
  this->displayColumns.push_back(eObservationDisplayX);
  this->displayColumns.push_back(eObservationDisplayY);
  this->displayColumns.push_back(eObservationDisplayZ);
  this->displayColumns.push_back(eObservationDisplayI);
  this->displayColumns.push_back(eObservationDisplayJ);
  this->displayColumns.push_back(eObservationDisplayK);
  this->displayColumns.push_back(eObservationDisplayIsSolved);
  this->displayColumns.push_back(eObservationDisplayVX);
  this->displayColumns.push_back(eObservationDisplayVY);
  this->displayColumns.push_back(eObservationDisplayVZ);
  this->displayColumns.push_back(eObservationDisplayV);
  this->displayColumns.push_back(eObservationDisplayVR);
  this->displayColumns.push_back(eObservationDisplayIsUsed);
  this->displayColumns.push_back(eObservationDisplayIsDummyPoint);
  this->displayColumns.push_back(eObservationDisplayReadingTime);
}

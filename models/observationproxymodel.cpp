#include "observationproxymodel.h"

#include <utility>
#include <vector>

#include "containers.h"

/*!
 * \brief ObservationProxyModel::ObservationProxyModel
 * \param parent
 */
ObservationProxyModel::ObservationProxyModel(QObject* parent)
    : QSortFilterProxyModel(parent) {}

/*!
 * \brief ObservationProxyModel::getObservationTableColumnConfig
 * \return
 */
const ObservationTableColumnConfig&
ObservationProxyModel::getObservationTableColumnConfig() const {
  return this->observationTableColumnConfig;
}

/*!
 * \brief ObservationProxyModel::setObservationTableColumnConfig
 * \param config
 */
void ObservationProxyModel::setObservationTableColumnConfig(
    const ObservationTableColumnConfig& config) {
  this->observationTableColumnConfig = config;
  this->invalidateFilter();
}

/*!
 * \brief ObservationProxyModel::filterAcceptsRow
 * \param source_row
 * \param source_parent
 * \return
 */
bool ObservationProxyModel::filterAcceptsRow(
    int source_row, const QModelIndex& source_parent) const {
  return true;
}

/*!
 * \brief ObservationProxyModel::filterAcceptsColumn
 * \param source_column
 * \param source_parent
 * \return
 */
bool ObservationProxyModel::filterAcceptsColumn(
    int source_column, const QModelIndex& source_parent) const {
  // check visibility of the column
  return this->observationTableColumnConfig.getColumnVisibility(
      (ObservationDisplayAttributes)source_column);
}

/*!
 * \brief ObservationProxyModel::lessThan
 * \param source_left
 * \param source_right
 * \return
 */
bool ObservationProxyModel::lessThan(const QModelIndex& source_left,
                                     const QModelIndex& source_right) const {
  int col = source_left.column();
  if ((ObservationDisplayAttributes)col != eObservationDisplayId) {
    return false;
  }

  ObservationModel* source_model =
      dynamic_cast<ObservationModel*>(this->sourceModel());
  if (source_model == nullptr) {
    return false;
  }

  // check if job is set
  if (source_model->getCurrentJob().isNull()) {
    return false;
  }

  // check feature, function and observation
  if (!source_model->getCurrentJob()->getActiveFeature().isNull() &&
      source_model->getCurrentJob()
              ->getActiveFeature()
              ->getFeature()
              ->getFunctions()
              .size() >= 1) {
    // fit function
    oi::Guarded<Function> function = source_model->getCurrentJob()
                                         ->getActiveFeature()
                                         ->getFeature()
                                         ->getFunctions()
                                         .at(0);
    std::vector<InputElement> inputElem =
        oi::value(function->getInputElements(), 0);

    if (source_right.isValid() && source_left.isValid() &&
        std::cmp_less(source_right.row(), inputElem.size()) &&
        std::cmp_less(source_left.row(), inputElem.size()) &&
        !inputElem.at(source_right.row()).observation.isNull() &&
        !inputElem.at(source_left.row()).observation.isNull()) {
      int fwRight = inputElem.at(source_right.row()).observation->getId();
      int fwLeft = inputElem.at(source_left.row()).observation->getId();

      return fwLeft < fwRight;
    }
  }
  return false;
}

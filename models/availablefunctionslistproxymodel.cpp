#include "availablefunctionslistproxymodel.h"

/*!
 * \brief AvailableFunctionsListProxyModel::AvailableFunctionsListProxyModel
 * \param parent
 */
AvailableFunctionsListProxyModel::AvailableFunctionsListProxyModel(
    QObject* parent)
    : QSortFilterProxyModel(parent) {}

/*!
 * \brief AvailableFunctionsListProxyModel::setFilter
 * \param type
 */
void AvailableFunctionsListProxyModel::setFilter(const FeatureTypes& type) {
  this->filter = type;
  this->invalidateFilter();
}

/*!
 * \brief AvailableFunctionsListProxyModel::getFunctionAtIndex
 * \param index
 * \return
 */
sys::Function AvailableFunctionsListProxyModel::getFunctionAtIndex(
    const int& index) const {
  // check index
  if (index < 0) {
    return sys::Function();
  }

  // get and cast source model
  AvailableFunctionsListModel* source_model =
      dynamic_cast<AvailableFunctionsListModel*>(this->sourceModel());
  if (source_model == nullptr) {
    return sys::Function();
  }

  // get the function at index
  QModelIndex functionIndex = this->index(index, 0);
  QModelIndex sourceIndex = this->mapToSource(functionIndex);
  sys::Function currentFunction = source_model->getFunctionAtIndex(
      source_model->index(sourceIndex.row(), 0));
  if (currentFunction.name().compare("") == 0) {
    return sys::Function();
  }

  return currentFunction;
}

/*!
 * \brief AvailableFunctionsListProxyModel::filterAcceptsRow
 * \param source_row
 * \param source_parent
 * \return
 */
bool AvailableFunctionsListProxyModel::filterAcceptsRow(
    int source_row, const QModelIndex& source_parent) const {
  // get and cast source model
  AvailableFunctionsListModel* source_model =
      dynamic_cast<AvailableFunctionsListModel*>(this->sourceModel());
  if (source_model == nullptr) {
    return false;
  }

  if (static_cast<int>(this->filter) < 0) {
    return false;
  }

  // get the function at index
  sys::Function currentFunction =
      source_model->getFunctionAtIndex(source_model->index(source_row, 0));
  if (currentFunction.name().compare("") == 0) {
    return false;
  }

  // decide purely from the function this row actually holds. Asking
  // PluginRepository instead would filter against the globally scanned plugin
  // set rather than whatever the source model was given, which diverges the
  // moment the model is populated from anywhere else (e.g.
  // ModelManager::testInit)
  return PluginRepository::isCreateFunctionFor(currentFunction, this->filter);
}

/*!
 * \brief AvailableFunctionsListProxyModel::filterAcceptsColumn
 * \param source_column
 * \param source_parent
 * \return
 */
bool AvailableFunctionsListProxyModel::filterAcceptsColumn(
    int source_column, const QModelIndex& source_parent) const {
  return true;
}

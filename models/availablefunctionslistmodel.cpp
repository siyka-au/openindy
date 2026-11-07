#include "availablefunctionslistmodel.h"

#include <vector>

#include "containers.h"

/*!
 * \brief AvailableFunctionsListModel::AvailableFunctionsListModel
 * \param parent
 */
AvailableFunctionsListModel::AvailableFunctionsListModel(QObject* parent)
    : QAbstractListModel(parent) {}

/*!
 * \brief AvailableFunctionsListModel::rowCount
 * \param parent
 * \return
 */
int AvailableFunctionsListModel::rowCount(const QModelIndex& parent) const {
  int rowCount = 0;

  // get the number of function plugins
  foreach (const sys::Plugin& plugin, this->plugins) {
    rowCount += plugin.functions().size();
  }

  return rowCount;
}

/*!
 * \brief AvailableFunctionsListModel::columnCount
 * \param parent
 * \return
 */
int AvailableFunctionsListModel::columnCount(const QModelIndex& parent) const {
  return 1;
}

/*!
 * \brief AvailableFunctionsListModel::data
 * \param index
 * \param role
 * \return
 */
QVariant AvailableFunctionsListModel::data(const QModelIndex& index,
                                           int role) const {
  if (!index.isValid()) return QVariant();

  if (role != Qt::DisplayRole) {
    return QVariant();
  }

  int rowCount = 0;

  // get the name of the function at index.row()
  foreach (const sys::Plugin& plugin, this->plugins) {
    foreach (const sys::Function& function, plugin.functions()) {
      if (rowCount == index.row()) {
        return function.name();
      }
      rowCount++;
    }
  }

  return QVariant();
}

/*!
 * \brief AvailableFunctionsListModel::headerData
 * \param section
 * \param orientation
 * \param role
 * \return
 */
QVariant AvailableFunctionsListModel::headerData(int section,
                                                 Qt::Orientation orientation,
                                                 int role) const {
  if ((Qt::DisplayRole == role) && (Qt::Horizontal == orientation) &&
      (0 <= section) && (section < 1)) {
    return "available functions";
  }
  return QVariant();
}

/*!
 * \brief AvailableFunctionsListModel::getFunctionAtIndex
 * \param index
 * \return
 */
sys::Function AvailableFunctionsListModel::getFunctionAtIndex(
    const QModelIndex& index) const {
  if (!index.isValid()) return sys::Function();

  int rowCount = 0;
  foreach (const sys::Plugin& plugin, this->plugins) {
    foreach (const sys::Function& function, plugin.functions()) {
      if (rowCount == index.row()) {
        return function;
      }
      rowCount++;
    }
  }

  return sys::Function();
}

/*!
 * \brief AvailableFunctionsListModel::getDefaultFunction
 * \param type
 * \return
 */
QPair<QString, QString> AvailableFunctionsListModel::getDefaultFunction(
    const FeatureTypes& type) const {
  // resolved against this->plugins (this model's own scoped list), not the
  // global PluginRepository - consistent with every other method here, and
  // what actually makes this model's result reflect whatever it was given
  // via setPlugins() (production data or an injected test list alike)
  QString name = Settings::get(
      QString("defaultFunction.%1").arg(getFeatureTypeName(type)));
  if (!name.isEmpty()) {
    foreach (const sys::Plugin& plugin, this->plugins) {
      foreach (const sys::Function& function, plugin.functions()) {
        if (function.name().compare(name) == 0 &&
            oi::contains(function.applicableFor(), type)) {
          QPair<QString, QString> result;
          result.first = function.name();
          result.second = function.plugin().filePath();
          return result;
        }
      }
    }
  }

  return QPair<QString, QString>();
}

/*!
 * \brief AvailableFunctionsListModel::getPlugins
 * \return
 */
const std::vector<sys::Plugin>& AvailableFunctionsListModel::getPlugins()
    const {
  return this->plugins;
}

/*!
 * \brief AvailableFunctionsListModel::setPlugins
 * \param plugins
 */
void AvailableFunctionsListModel::setPlugins(
    const std::vector<sys::Plugin>& plugins) {
  this->plugins = plugins;
  this->updateModel();
}

/*!
 * \brief AvailableFunctionsListModel::updateModel
 */
void AvailableFunctionsListModel::updateModel() {
  emit this->layoutAboutToBeChanged();
  emit this->layoutChanged();
}

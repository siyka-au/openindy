#include "measurementconfigurationproxymodel.h"

#include <vector>

/*!
 * \brief MeasurementConfigurationProxyModel::MeasurementConfigurationProxyModel
 * \param parent
 */
MeasurementConfigurationProxyModel::MeasurementConfigurationProxyModel(
    QObject* parent)
    : QSortFilterProxyModel(parent) {}

/*!
 * \brief MeasurementConfigurationProxyModel::setFilter
 * \param showAll
 */
void MeasurementConfigurationProxyModel::setFilter(const bool& showAll) {
  this->resetFilter();

  this->filterType = showAll ? MeasurementConfigurationFilterTypes::
                                   eAll_MeasurementConfigurationFilter
                             : MeasurementConfigurationFilterTypes::
                                   eNo_MeasurementConfigurationFilter;

  this->invalidateFilter();
  this->sort(0);
}
void MeasurementConfigurationProxyModel::setFilter(
    const std::vector<ElementTypes> neededElements, FeatureTypes typeOfFeature,
    std::vector<FeatureTypes> applicableFor,
    const SensorType& activeSensorType) {
  this->resetFilter();

  this->filterType = MeasurementConfigurationFilterTypes::
      eCreateFeature_MeasurementConfigurationFilter;
  this->neededElements = neededElements;
  this->typeOfFeature = typeOfFeature;
  this->functionIsApplicableFor = applicableFor;
  this->activeSensorType = activeSensorType;

  this->invalidateFilter();
  this->sort(0);
}
void MeasurementConfigurationProxyModel::setFilterProjectConfig() {
  this->resetFilter();

  this->filterType = MeasurementConfigurationFilterTypes::
      eProject_MeasurementConfigurationFilter;

  this->invalidateFilter();
  this->sort(0);
}
void MeasurementConfigurationProxyModel::setFilterUserConfig() {
  this->resetFilter();

  this->filterType =
      MeasurementConfigurationFilterTypes::eUser_MeasurementConfigurationFilter;

  this->invalidateFilter();
  this->sort(0);
}

void MeasurementConfigurationProxyModel::resetFilter() {
  this->filterType =
      MeasurementConfigurationFilterTypes::eNo_MeasurementConfigurationFilter;

  this->neededElements.clear();
  this->typeOfFeature = FeatureTypes::eUndefinedFeature;
  this->functionIsApplicableFor.clear();
  this->activeSensorType = SensorType();
  this->sort(0);
}

/*!
 * \brief MeasurementConfigurationProxyModel::headerData
 * \param section
 * \param orientation
 * \param role
 * \return
 */
QVariant MeasurementConfigurationProxyModel::headerData(
    int section, Qt::Orientation orientation, int role) const {
  if ((Qt::DisplayRole == role) && (Qt::Horizontal == orientation) &&
      (0 <= section) && (section < columnCount())) {
    return "measurement configurations";
  }
  return QVariant();
}

/*!
 * \brief MeasurementConfigurationProxyModel::filterAcceptsRow
 * \param source_row
 * \param source_parent
 * \return
 */
bool MeasurementConfigurationProxyModel::filterAcceptsRow(
    int source_row, const QModelIndex& source_parent) const {
  // get and check source model
  MeasurementConfigurationModel* sourceModel =
      static_cast<MeasurementConfigurationModel*>(this->sourceModel());
  if (sourceModel == nullptr) {
    return false;
  }

  const std::vector<MeasurementConfig> configs =
      sourceModel->getMeasurementConfigurationManager()->getConfigs();
  if (source_row < 0 || source_row >= configs.size()) {
    return false;
  }

  MeasurementConfig mc = configs.at(source_row);
  const bool isProject = mc.isValid() && mc.isProjectConfig();
  const bool isUser = mc.isValid() && mc.isUserConfig();

  // an untagged config (user/project-authored generically, or predating
  // the tag) stays visible regardless of active sensor - only a config
  // that's explicitly tagged for a *different* type gets narrowed out.
  // With no active sensor known yet, don't filter by type at all.
  if (this->activeSensorType.isValid() && mc.getSensorType().isValid() &&
      !(mc.getSensorType() == this->activeSensorType)) {
    return false;
  }

  switch (this->filterType) {
    case MeasurementConfigurationFilterTypes::
        eProject_MeasurementConfigurationFilter:
      return isProject;
    case MeasurementConfigurationFilterTypes::
        eUser_MeasurementConfigurationFilter:
      return isUser;
    case MeasurementConfigurationFilterTypes::
        eCreateFeature_MeasurementConfigurationFilter:
      if (isUser) {
        int i = 0;
        foreach (const MeasurementConfig& c, configs) {
          if (++i == source_row) {
            continue;
          }
          if (c.isProjectConfig() && mc.getName().compare(c.getName()) == 0) {
            return false;  // project config with same name exits
          }
        }
      }
      // *********************
      // fall through intended
      // *********************
    case MeasurementConfigurationFilterTypes::
        eNo_MeasurementConfigurationFilter:
    case MeasurementConfigurationFilterTypes::
        eAll_MeasurementConfigurationFilter:
    default:
      if (isUser || isProject) {
        if (this->neededElements.empty()) {
          return true;
        }

        for (ElementTypes elementType : this->neededElements) {
          if (mc.applicableFor(elementType, this->functionIsApplicableFor)) {
            return true;
          }
        }
      }
      return false;
  }

  return false;
}

/*!
 * \brief MeasurementConfigurationProxyModel::filterAcceptsColumn
 * \param source_row
 * \param source_parent
 * \return
 */
bool MeasurementConfigurationProxyModel::filterAcceptsColumn(
    int source_row, const QModelIndex& source_parent) const {
  return true;
}

bool MeasurementConfigurationProxyModel::lessThan(
    const QModelIndex& left, const QModelIndex& right) const {
  MeasurementConfigurationModel* sourceModel =
      static_cast<MeasurementConfigurationModel*>(this->sourceModel());
  if (sourceModel == nullptr) {
    return false;
  }
  QVariant leftData = sourceModel->data(left);
  QVariant rightData = sourceModel->data(right);

  return leftData.toString().compare(rightData.toString(),
                                     Qt::CaseInsensitive) < 0;
}

QVariant MeasurementConfigurationProxyModel::data(const QModelIndex& index,
                                                  int role) const {
  MeasurementConfigurationModel* sourceModel =
      static_cast<MeasurementConfigurationModel*>(this->sourceModel());
  if (sourceModel == nullptr) {
    return QVariant();
  }
  QVariant data = sourceModel->data(this->mapToSource(index), role);

  if (role != Qt::DisplayRole) {
    return data;
  }

  if (this->filterType == MeasurementConfigurationFilterTypes::
                              eCreateFeature_MeasurementConfigurationFilter &&
      sourceModel->isUserConfig(this->mapToSource(index))) {
    return QString("%1 [user]").arg(data.toString());
  }

  return data;
}

MeasurementConfig MeasurementConfigurationProxyModel::getMeasurementConfig(
    const QModelIndex& index) const {
  MeasurementConfigurationModel* sourceModel =
      static_cast<MeasurementConfigurationModel*>(this->sourceModel());

  return sourceModel->getMeasurementConfig(this->mapToSource(index));
}

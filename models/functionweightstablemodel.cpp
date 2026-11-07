#include "functionweightstablemodel.h"

#include <vector>

#include "containers.h"
#include "qtjobadapter.h"

/*!
 * \brief FunctionWeightsTableModel::FunctionWeightsTableModel
 * \param parent
 */
FunctionWeightsTableModel::FunctionWeightsTableModel(oi::Guarded<OiJob> job,
                                                     QObject* parent)
    : QAbstractTableModel(parent), functionPosition(-1) {
  this->setCurrentJob(job);
}

/*!
 * \brief FunctionWeightsTableModel::FunctionWeightsTableModel
 * \param parent
 */
FunctionWeightsTableModel::FunctionWeightsTableModel(QObject* parent)
    : QAbstractTableModel(parent), functionPosition(-1) {}

/*!
 * \brief FunctionWeightsTableModel::getCurrentJob
 * \return
 */
const oi::Guarded<OiJob>& FunctionWeightsTableModel::getCurrentJob() const {
  return this->currentJob;
}

/*!
 * \brief FunctionWeightsTableModel::setCurrentJob
 * \param job
 */
void FunctionWeightsTableModel::setCurrentJob(const oi::Guarded<OiJob>& job) {
  if (!job.isNull()) {
    // disconnect current job
    if (!this->currentJob.isNull()) {
      this->disconnectJob();
    }

    this->currentJob = job;
    this->connectJob();
    this->updateModel();
  }
}

/*!
 * \brief FunctionWeightsTableModel::rowCount
 * \param parent
 * \return
 */
int FunctionWeightsTableModel::rowCount(const QModelIndex& parent) const {
  // check current job
  if (this->currentJob.isNull()) {
    return 0;
  }

  // get and check active feature
  if (this->currentJob->getActiveFeature().isNull() ||
      this->currentJob->getActiveFeature()->getFeature().isNull()) {
    return 0;
  }
  // check feature for valid trafo param
  oi::Guarded<FeatureWrapper> feature = this->currentJob->getActiveFeature();
  if (feature.isNull() || feature->getTrafoParam().isNull()) {
    return 0;
  }

  // check selected function position
  if (this->functionPosition < 0 ||
      this->functionPosition >=
          feature->getTrafoParam()->getFunctions().size() ||
      feature->getTrafoParam()
          ->getFunctions()
          .at(this->functionPosition)
          .isNull()) {
    return 0;
  }
  Function* function =
      feature->getTrafoParam()->getFunctions().at(this->functionPosition);

  if (function->getInputElements().size() >= 1) {
    return oi::value(function->getInputElements(), 0).size();
  }
  return 0;
}

/*!
 * \brief FunctionWeightsTableModel::columnCount
 * \param parent
 * \return
 */
int FunctionWeightsTableModel::columnCount(const QModelIndex& parent) const {
  return 4;
}

/*!
 * \brief FunctionWeightsTableModel::data
 * \param index
 * \param role
 * \return
 */
QVariant FunctionWeightsTableModel::data(const QModelIndex& index,
                                         int role) const {
  // check current job and model index
  if (this->currentJob.isNull() || !index.isValid()) {
    return QVariant();
  }

  // get and check active feature
  if (this->currentJob->getActiveFeature().isNull() ||
      this->currentJob->getActiveFeature()->getFeature().isNull()) {
    return QVariant();
  }
  oi::Guarded<Feature> feature =
      this->currentJob->getActiveFeature()->getFeature();

  // check selected function position
  if (this->functionPosition < 0 ||
      this->functionPosition >= feature->getFunctions().size() ||
      feature->getFunctions().at(this->functionPosition).isNull()) {
    return QVariant();
  }
  Function* function = feature->getFunctions().at(this->functionPosition);

  std::vector<InputElement> inputElem =
      oi::value(function->getInputElements(), 0);

  // get feature from id of inputElement
  FeatureWrapper* fw =
      this->currentJob->getFeatureById(inputElem.at(index.row()).id);

  if (role == Qt::DisplayRole) {
    switch (index.column()) {
      case 0:
        return fw->getFeature()->getFeatureName();
    }

  } else if (role == Qt::CheckStateRole) {
    switch (index.column()) {
      case 1:
        if (oi::contains(inputElem.at(index.row()).ignoredDestinationParams,
                         GeometryParameters::eUnknownX)) {
          return Qt::Unchecked;
        } else {
          return Qt::Checked;
        }
      case 2:
        if (oi::contains(inputElem.at(index.row()).ignoredDestinationParams,
                         GeometryParameters::eUnknownY)) {
          return Qt::Unchecked;
        } else {
          return Qt::Checked;
        }
      case 3:
        if (oi::contains(inputElem.at(index.row()).ignoredDestinationParams,
                         GeometryParameters::eUnknownZ)) {
          return Qt::Unchecked;
        } else {
          return Qt::Checked;
        }
      default:
        break;
    }
  }
  return QVariant();
}

/*!
 * \brief FunctionWeightsTableModel::headerData
 * \param section
 * \param orientation
 * \param role
 * \return
 */
QVariant FunctionWeightsTableModel::headerData(int section,
                                               Qt::Orientation orientation,
                                               int role) const {
  if ((Qt::DisplayRole == role) && (Qt::Horizontal == orientation) &&
      (0 <= section) && (section < columnCount())) {
    switch (section) {
      case 0:
        return "feature name";
        break;
      case 1:
        return "use X";
        break;
      case 2:
        return "use Y";
        break;
      case 3:
        return "use Z";
        break;
    }
  }

  return QVariant();
}

/*!
 * \brief FunctionWeightsTableModel::flags
 * \param index
 * \return
 */
Qt::ItemFlags FunctionWeightsTableModel::flags(const QModelIndex& index) const {
  // get parent flags
  Qt::ItemFlags myFlags = QAbstractTableModel::flags(index);

  // check index
  if (!index.isValid()) {
    return myFlags;
  }
  switch (index.column()) {
    case 1:
      return (myFlags | Qt::ItemIsEditable | Qt::ItemIsUserCheckable);
    case 2:
      return (myFlags | Qt::ItemIsEditable | Qt::ItemIsUserCheckable);
    case 3:
      return (myFlags | Qt::ItemIsEditable | Qt::ItemIsUserCheckable);
  }
  return myFlags;
}

/*!
 * \brief FunctionWeightsTableModel::setData
 * \param index
 * \param value
 * \param role
 * \return
 */
bool FunctionWeightsTableModel::setData(const QModelIndex& index,
                                        const QVariant& value, int role) {
  // check current job and model index
  if (this->currentJob.isNull() || !index.isValid()) {
    return false;
  }

  // get the feature to index.row
  if (this->currentJob->getFeatureCount() <= index.row()) {
    return false;
  }

  // get and check column index
  int column = index.column();
  if (column < 0 || 3 < column) {
    return false;
  }

  // get and check active feature
  if (this->currentJob->getActiveFeature().isNull() ||
      this->currentJob->getActiveFeature()->getFeature().isNull()) {
    return false;
  }
  oi::Guarded<Feature> feature =
      this->currentJob->getActiveFeature()->getFeature();

  // check selected function position
  if (this->functionPosition < 0 ||
      this->functionPosition >= feature->getFunctions().size() ||
      feature->getFunctions().at(this->functionPosition).isNull()) {
    return false;
  }
  Function* function = feature->getFunctions().at(this->functionPosition);

  std::vector<InputElement> inputElem =
      oi::value(function->getInputElements(), 0);

  FeatureWrapper* fw =
      this->currentJob->getFeatureById(inputElem.at(index.row()).id);

  if (fw->getGeometry().isNull()) {
    return false;
  }

  if (role == Qt::CheckStateRole) {
    switch (index.column()) {
      case 1: {
        bool useX = value.toBool();

        if (fw->getGeometry()
                ->getOwnerDescription()
                .has_value()) {  // save the ignored parameters for actual
                                 // features

          for (auto& i : inputElem) {
            if (fw->getGeometry()->getId() ==
                i.id) {  // check id of current feature and input element
              if (useX) {
                // remove from ignore list to use
                if (oi::contains(i.ignoredDestinationParams,
                                 GeometryParameters::eUnknownX)) {
                  oi::removeOne(i.ignoredDestinationParams,
                                GeometryParameters::eUnknownX);
                }
                // save in current feature function
                this->currentJob->getActiveFeature()
                    ->getFeature()
                    ->getFunctions()[this->functionPosition]
                    ->replaceInputElement(i, this->functionPosition);
                this->updateModel();
                return true;
              } else {
                // add to ignore list
                if (!oi::contains(i.ignoredDestinationParams,
                                  GeometryParameters::eUnknownX)) {
                  i.ignoredDestinationParams.push_back(
                      GeometryParameters::eUnknownX);
                }
                // save in current feature function
                this->currentJob->getActiveFeature()
                    ->getFeature()
                    ->getFunctions()[this->functionPosition]
                    ->replaceInputElement(i, this->functionPosition);
                this->updateModel();
                return true;
              }
            }
          }
        }
        return false;
      }
      case 2: {
        bool useY = value.toBool();

        if (fw->getGeometry()
                ->getOwnerDescription()
                .has_value()) {  // save the ignored parameters for actual
                                 // features

          for (auto& i : inputElem) {
            if (fw->getGeometry()->getId() == i.id) {
              if (useY) {
                // remove from ignore list to use
                if (oi::contains(i.ignoredDestinationParams,
                                 GeometryParameters::eUnknownY)) {
                  oi::removeOne(i.ignoredDestinationParams,
                                GeometryParameters::eUnknownY);
                }
                // save in current feature function
                this->currentJob->getActiveFeature()
                    ->getFeature()
                    ->getFunctions()[this->functionPosition]
                    ->replaceInputElement(i, this->functionPosition);
                this->updateModel();
                return true;
              } else {
                // add to ignore list
                if (!oi::contains(i.ignoredDestinationParams,
                                  GeometryParameters::eUnknownY)) {
                  i.ignoredDestinationParams.push_back(
                      GeometryParameters::eUnknownY);
                }
                // save in current feature function
                this->currentJob->getActiveFeature()
                    ->getFeature()
                    ->getFunctions()[this->functionPosition]
                    ->replaceInputElement(i, this->functionPosition);
                this->updateModel();
                return true;
              }
            }
          }
        }
        return false;
      }
      case 3: {
        bool useZ = value.toBool();

        if (fw->getGeometry()
                ->getOwnerDescription()
                .has_value()) {  // save the ignored parameters for actual
                                 // features

          for (auto& i : inputElem) {
            if (fw->getGeometry()->getId() == i.id) {
              if (useZ) {
                // remove from ignore list to use
                if (oi::contains(i.ignoredDestinationParams,
                                 GeometryParameters::eUnknownZ)) {
                  oi::removeOne(i.ignoredDestinationParams,
                                GeometryParameters::eUnknownZ);
                }
                // save in current feature function
                this->currentJob->getActiveFeature()
                    ->getFeature()
                    ->getFunctions()[this->functionPosition]
                    ->replaceInputElement(i, this->functionPosition);
                this->updateModel();
                return true;
              } else {
                // add to ignore list
                if (!oi::contains(i.ignoredDestinationParams,
                                  GeometryParameters::eUnknownZ)) {
                  i.ignoredDestinationParams.push_back(
                      GeometryParameters::eUnknownZ);
                }
                // save in current feature function
                this->currentJob->getActiveFeature()
                    ->getFeature()
                    ->getFunctions()[this->functionPosition]
                    ->replaceInputElement(i, this->functionPosition);
                this->updateModel();
                return true;
              }
            }
          }
        }
        return false;
      }
      default:
        return false;
        break;
    }
  }
  return false;
}

/*!
 * \brief FunctionWeightsTableModel::setFunctionPosition
 * \param pos
 */
void FunctionWeightsTableModel::setFunctionPosition(int pos) {
  this->functionPosition = pos;
  this->updateModel();
}

/*!
 * \brief FunctionWeightsTableModel::updateModel
 */
void FunctionWeightsTableModel::updateModel() {
  emit this->layoutAboutToBeChanged();
  emit this->layoutChanged();
}

/*!
 * \brief FunctionWeightsTableModel::connectJob
 */
void FunctionWeightsTableModel::connectJob() {
  QObject::connect(QtJobAdapter::of(this->currentJob.data()),
                   &QtJobAdapter::activeFeatureChanged, this,
                   &FunctionWeightsTableModel::updateModel, Qt::AutoConnection);
  QObject::connect(QtJobAdapter::of(this->currentJob.data()),
                   &QtJobAdapter::featureFunctionsChanged, this,
                   &FunctionWeightsTableModel::updateModel, Qt::AutoConnection);
}

/*!
 * \brief FunctionWeightsTableModel::disconnectJob
 */
void FunctionWeightsTableModel::disconnectJob() {
  QObject::disconnect(QtJobAdapter::of(this->currentJob.data()),
                      &QtJobAdapter::activeFeatureChanged, this,
                      &FunctionWeightsTableModel::updateModel);
  QObject::disconnect(QtJobAdapter::of(this->currentJob.data()),
                      &QtJobAdapter::featureFunctionsChanged, this,
                      &FunctionWeightsTableModel::updateModel);
}

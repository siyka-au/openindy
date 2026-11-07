#include "usedelementsmodel.h"

#include <utility>
#include <vector>

#include "containers.h"
#include "jobconnect.h"
#include "qtconvert.h"
#include "qtfunctionadapter.h"
#include "qtjobadapter.h"

/*!
 * \brief UsedElementsModel::UsedElementsModel
 * \param parent
 */
UsedElementsModel::UsedElementsModel(QObject* parent)
    : QAbstractListModel(parent) {
  this->resetSelectedFunctionPosition();
}

/*!
 * \brief UsedElementsModel::rowCount
 * \param parent
 * \return
 */
int UsedElementsModel::rowCount(const QModelIndex& parent) const {
  // check current job
  if (this->currentJob.isNull()) {
    return 0;
  }

  // get and check active feature
  if (this->currentJob->getActiveFeature().isNull() ||
      this->currentJob->getActiveFeature()->getFeature().isNull()) {
    return 0;
  }
  Feature* feature = this->currentJob->getActiveFeature()->getFeature();

  // check selected function position
  if (this->functionPosition < 0 ||
      std::cmp_greater_equal(this->functionPosition,
                             feature->getFunctions().size()) ||
      feature->getFunctions().at(this->functionPosition).isNull()) {
    return 0;
  }
  Function* function = feature->getFunctions().at(this->functionPosition);

  NeededElement neededElement =
      function->getNeededElements().at(this->neededElementIndex);
  const int inputElementKey =
      this->neededElementIndex;  // D12: position is always the real
                                 // NeededElement index now

  // check input elements
  const auto elements = function->getInputElements().find(inputElementKey);
  if (elements == function->getInputElements().cend()) {
    return 0;
  }
  return static_cast<int>(elements->second.size());
}

/*!
 * \brief UsedElementsModel::columnCount
 * \param parent
 * \return
 */
int UsedElementsModel::columnCount(const QModelIndex& parent) const {
  return 1;
}

/*!
 * \brief UsedElementsModel::data
 * \param index
 * \param role
 * \return
 */
QVariant UsedElementsModel::data(const QModelIndex& index, int role) const {
  if (!index.isValid()) return QVariant();

  // check current job
  if (this->currentJob.isNull()) {
    return QVariant();
  }

  // get and check active feature
  if (this->currentJob->getActiveFeature().isNull() ||
      this->currentJob->getActiveFeature()->getFeature().isNull()) {
    return QVariant();
  }
  Feature* feature = this->currentJob->getActiveFeature()->getFeature();

  // check selected function position
  if (this->functionPosition < 0 ||
      std::cmp_greater_equal(this->functionPosition,
                             feature->getFunctions().size()) ||
      feature->getFunctions().at(this->functionPosition).isNull()) {
    return QVariant();
  }
  Function* function = feature->getFunctions().at(this->functionPosition);

  NeededElement neededElement =
      function->getNeededElements().at(this->neededElementIndex);
  const int inputElementKey =
      this->neededElementIndex;  // D12: position is always the real
                                 // NeededElement index now
  // check and get input element
  const auto elements = function->getInputElements().find(inputElementKey);
  if (elements == function->getInputElements().cend() || index.row() < 0 ||
      static_cast<std::size_t>(index.row()) >= elements->second.size()) {
    return QVariant();
  }
  InputElement element =
      elements->second.at(static_cast<std::size_t>(index.row()));

  if (role == Qt::DisplayRole) {
    if (!element.geometry.isNull() &&
        !element.geometry->getOwnerDescription().has_value() &&
        !element.geometry->getNominalSystem().isNull()) {
      return QString("%1 - nominal (%3)")
          .arg(element.geometry->getFeatureName())
          .arg(element.geometry->getNominalSystem()->getFeatureName());
    } else if (!element.geometry.isNull() &&
               element.geometry->getOwnerDescription().has_value()) {
      return QString("%1 - actual").arg(element.geometry->getFeatureName());
    } else if (!element.station.isNull()) {
      return toQString(element.station->getFeatureName());
    } else if (!element.coordSystem.isNull()) {
      return toQString(element.coordSystem->getFeatureName());
    } else if (!element.trafoParam.isNull()) {
      return toQString(element.trafoParam->getFeatureName());
    } else if (!element.observation.isNull() &&
               !element.observation->getReading().isNull()) {
      if (!element.observation->getMeasuredTargetGeometry().isNull()) {
        return "id:" + QString::number(element.observation->getId()) + " " +
               toQString(element.observation->getMeasuredTargetGeometry()
                             ->getFeatureName());
      }
      return "id:" + QString::number(element.observation->getId()) + " " +
             toQString(element.observation->getStation()->getFeatureName());

    } else if (!element.polarReading.isNull()) {
      if (!element.polarReading->getObservation()
               ->getMeasuredTargetGeometry()
               .isNull()) {
        return "id:" + QString::number(element.polarReading->getId()) +
               toQString(element.polarReading->getObservation()
                             ->getMeasuredTargetGeometry()
                             ->getFeatureName());
      }
      return "id:" + QString::number(element.polarReading->getId()) +
             toQString(element.polarReading->getObservation()
                           ->getStation()
                           ->getFeatureName());

    } else if (!element.directionReading.isNull()) {
      if (!element.directionReading->getObservation()
               ->getMeasuredTargetGeometry()
               .isNull()) {
        return "id:" + QString::number(element.directionReading->getId()) +
               toQString(element.directionReading->getObservation()
                             ->getMeasuredTargetGeometry()
                             ->getFeatureName());
      }
      return "id:" + QString::number(element.directionReading->getId()) +
             toQString(element.directionReading->getObservation()
                           ->getStation()
                           ->getFeatureName());

    } else if (!element.distanceReading.isNull()) {
      if (!element.distanceReading->getObservation()
               ->getMeasuredTargetGeometry()
               .isNull()) {
        return "id:" + QString::number(element.distanceReading->getId()) +
               toQString(element.distanceReading->getObservation()
                             ->getMeasuredTargetGeometry()
                             ->getFeatureName());
      }
      return "id:" + QString::number(element.distanceReading->getId()) +
             toQString(element.distanceReading->getObservation()
                           ->getStation()
                           ->getFeatureName());

    } else if (!element.cartesianReading.isNull()) {
      if (!element.cartesianReading->getObservation()
               ->getMeasuredTargetGeometry()
               .isNull()) {
        return "id:" + QString::number(element.cartesianReading->getId()) +
               toQString(element.cartesianReading->getObservation()
                             ->getMeasuredTargetGeometry()
                             ->getFeatureName());
      }
      return "id:" + QString::number(element.cartesianReading->getId()) +
             toQString(element.cartesianReading->getObservation()
                           ->getStation()
                           ->getFeatureName());

    } else if (!element.temperatureReading.isNull()) {
      if (!element.temperatureReading->getObservation()
               ->getMeasuredTargetGeometry()
               .isNull()) {
        return "id:" + QString::number(element.temperatureReading->getId()) +
               toQString(element.temperatureReading->getObservation()
                             ->getMeasuredTargetGeometry()
                             ->getFeatureName());
      }
      return "id:" + QString::number(element.temperatureReading->getId()) +
             toQString(element.temperatureReading->getObservation()
                           ->getStation()
                           ->getFeatureName());

    } else if (!element.levelReading.isNull()) {
      if (!element.levelReading->getObservation()
               ->getMeasuredTargetGeometry()
               .isNull()) {
        return "id:" + QString::number(element.levelReading->getId()) +
               toQString(element.levelReading->getObservation()
                             ->getMeasuredTargetGeometry()
                             ->getFeatureName());
      }
      return "id:" + QString::number(element.levelReading->getId()) +
             toQString(element.levelReading->getObservation()
                           ->getStation()
                           ->getFeatureName());

    } else if (!element.undefinedReading.isNull()) {
      if (!element.undefinedReading->getObservation()
               ->getMeasuredTargetGeometry()
               .isNull()) {
        return "id:" + QString::number(element.undefinedReading->getId()) +
               toQString(element.undefinedReading->getObservation()
                             ->getMeasuredTargetGeometry()
                             ->getFeatureName());
      }
      return "id:" + QString::number(element.undefinedReading->getId()) +
             toQString(element.undefinedReading->getObservation()
                           ->getStation()
                           ->getFeatureName());
    }
    return QVariant();

  } else if (role == Qt::DecorationRole) {
    // get and check icon path
    const QString iconPath =
        toQString(getElementTypeIconPath(element.typeOfElement));
    if (iconPath.compare("") == 0) {
      return QVariant();
    }

    QPixmap pix(iconPath);
    return pix.scaledToHeight(20, Qt::SmoothTransformation);
  }

  return QVariant();
}

/*!
 * \brief UsedElementsModel::headerData
 * \param section
 * \param orientation
 * \param role
 * \return
 */
QVariant UsedElementsModel::headerData(int section, Qt::Orientation orientation,
                                       int role) const {
  if ((Qt::DisplayRole == role) && (Qt::Horizontal == orientation) &&
      (0 <= section) && (section < 1)) {
    return "used elements";
  }
  return QVariant();
}

/*!
 * \brief UsedElementsModel::removeUsedElements
 * \param selection
 */
void UsedElementsModel::removeUsedElements(const QModelIndexList& selection) {
  // check current job
  if (this->currentJob.isNull()) {
    return;
  }

  // get and check active feature
  if (this->currentJob->getActiveFeature().isNull() ||
      this->currentJob->getActiveFeature()->getFeature().isNull()) {
    return;
  }
  Feature* feature = this->currentJob->getActiveFeature()->getFeature();

  // check selected function position
  if (this->functionPosition < 0 ||
      std::cmp_greater_equal(this->functionPosition,
                             feature->getFunctions().size()) ||
      feature->getFunctions().at(this->functionPosition).isNull()) {
    return;
  }
  Function* function = feature->getFunctions().at(this->functionPosition);

  NeededElement neededElement =
      function->getNeededElements().at(this->neededElementIndex);
  const int inputElementKey =
      this->neededElementIndex;  // D12: position is always the real
                                 // NeededElement index now
  // check and get input elements
  if (!function->getInputElements().contains(inputElementKey)) {
    return;
  }

  std::vector<InputElement> inputElements =
      oi::value(function->getInputElements(), inputElementKey);

  // remove all selected elements
  foreach (const QModelIndex& index, selection) {
    // check index
    if (!index.isValid() ||
        std::cmp_greater_equal(index.row(), inputElements.size())) {
      return;
    }

    emit this->startRemoveUsedElements(this->currentJob->getActiveFeature(),
                                       this->functionPosition, inputElementKey,
                                       inputElements.at(index.row()).id);
  }
}

/*!
 * \brief UsedElementsModel::getCurrentJob
 * \return
 */
const oi::Guarded<OiJob>& UsedElementsModel::getCurrentJob() const {
  return this->currentJob;
}

/*!
 * \brief UsedElementsModel::setCurrentJob
 * \param job
 */
void UsedElementsModel::setCurrentJob(const oi::Guarded<OiJob>& job) {
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
 * \brief UsedElementsModel::setSelectedFunctionPosition
 * \param functionPosition
 * \param neededElementIndex
 */
void UsedElementsModel::setSelectedFunctionPosition(
    const int& functionPosition, const int& neededElementIndex) {
  // reset old function position
  this->resetSelectedFunctionPosition();

  // set function index and input element index
  this->functionPosition = functionPosition;
  this->neededElementIndex = neededElementIndex;

  // get and check active feature
  if (this->currentJob->getActiveFeature().isNull() ||
      this->currentJob->getActiveFeature()->getFeature().isNull()) {
    return;
  }
  Feature* feature = this->currentJob->getActiveFeature()->getFeature();

  // check selected function position
  if (this->functionPosition < 0 ||
      std::cmp_greater_equal(this->functionPosition,
                             feature->getFunctions().size()) ||
      feature->getFunctions().at(this->functionPosition).isNull()) {
    return;
  }
  this->connectedFunction = feature->getFunctions().at(this->functionPosition);

  // connect the specified function
  QObject::connect(QtFunctionAdapter::of(this->connectedFunction),
                   &QtFunctionAdapter::inputElementsChanged, this,
                   &UsedElementsModel::updateModel, Qt::AutoConnection);

  this->updateModel();
}

/*!
 * \brief UsedElementsModel::updateModel
 */
void UsedElementsModel::updateModel() {
  emit this->layoutAboutToBeChanged();
  emit this->layoutChanged();
}

/*!
 * \brief UsedElementsModel::connectJob
 */
void UsedElementsModel::connectJob() {
  QObject::connect(QtJobAdapter::of(this->currentJob.data()),
                   &QtJobAdapter::activeFeatureChanged, this,
                   &UsedElementsModel::resetSelectedFunctionPosition,
                   Qt::AutoConnection);
  QObject::connect(QtJobAdapter::of(this->currentJob.data()),
                   &QtJobAdapter::activeFeatureChanged, this,
                   &UsedElementsModel::updateModel, Qt::AutoConnection);

  connectToJob(this, &UsedElementsModel::startRemoveUsedElements,
               this->currentJob, &OiJob::removeInputElement);
}

/*!
 * \brief UsedElementsModel::disconnectJob
 */
void UsedElementsModel::disconnectJob() {
  QObject::disconnect(QtJobAdapter::of(this->currentJob.data()),
                      &QtJobAdapter::activeFeatureChanged, this,
                      &UsedElementsModel::resetSelectedFunctionPosition);
  QObject::disconnect(QtJobAdapter::of(this->currentJob.data()),
                      &QtJobAdapter::activeFeatureChanged, this,
                      &UsedElementsModel::updateModel);

  disconnectFromJob(this);
}

/*!
 * \brief UsedElementsModel::resetSelectedFunctionPosition
 * Reset the model
 */
void UsedElementsModel::resetSelectedFunctionPosition() {
  // reset function index and input element index
  this->functionPosition = -1;
  this->neededElementIndex = -1;

  // disconnect function
  if (!this->connectedFunction.isNull()) {
    QObject::disconnect(QtFunctionAdapter::of(this->connectedFunction),
                        &QtFunctionAdapter::inputElementsChanged, this,
                        &UsedElementsModel::updateModel);
    this->connectedFunction = oi::Guarded<Function>(nullptr);
  }
}

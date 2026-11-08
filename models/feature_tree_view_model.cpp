#include "feature_tree_view_model.h"

#include "qt_convert.h"
#include "qt_job_adapter.h"

namespace {
constexpr int kIconHeight = 20;  // pixels
}  // namespace

/*!
 * \brief FeatureTreeViewModel::FeatureTreeViewModel
 * \param job
 * \param parent
 */
FeatureTreeViewModel::FeatureTreeViewModel(oi::Guarded<OiJob> job,
                                           QObject* parent)
    : QAbstractItemModel(parent) {
  this->rootItem = new FeatureTreeItem("features");
  this->setCurrentJob(job);
}

/*!
 * \brief FeatureTreeViewModel::FeatureTreeViewModel
 * \param parent
 */
FeatureTreeViewModel::FeatureTreeViewModel(QObject* parent) {
  this->rootItem = new FeatureTreeItem("features");
}

/*!
 * \brief FeatureTreeViewModel::rowCount
 * \param parent
 * \return
 */
int FeatureTreeViewModel::rowCount(const QModelIndex& parent) const {
  FeatureTreeItem* childItem = nullptr;
  if (!parent.isValid()) {
    childItem = rootItem;
  } else {
    childItem = static_cast<FeatureTreeItem*>(parent.internalPointer());
  }
  if (childItem != nullptr) {
    return childItem->getChildCount();
  }
  return 0;
}

/*!
 * \brief FeatureTreeViewModel::columnCount
 * \param parent
 * \return
 */
int FeatureTreeViewModel::columnCount(const QModelIndex& parent) const {
  return 1;
}

/*!
 * \brief FeatureTreeViewModel::data
 * \param index
 * \param role
 * \return
 */
QVariant FeatureTreeViewModel::data(const QModelIndex& index, int role) const {
  if (!index.isValid()) return QVariant();

  FeatureTreeItem* item =
      static_cast<FeatureTreeItem*>(index.internalPointer());

  if (item != nullptr) {
    if (role == Qt::DisplayRole) {  // return display text for tree view item

      return item->getDisplayValue();

    } else if (role == Qt::DecorationRole) {  // return icon for tree view item

      if (item->getIsFeature() && !item->getFeature().isNull()) {
        // get and check icon path
        const QString iconPath = toQString(
            getFeatureTypeIconPath(item->getFeature()->getFeatureTypeEnum()));
        if (iconPath.compare("") == 0) {
          return QVariant();
        }

        QPixmap pix(iconPath);
        return pix.scaledToHeight(kIconHeight, Qt::SmoothTransformation);
      }
    }
  }

  return QVariant();
}

/*!
 * \brief FeatureTreeViewModel::index
 * Create model index for every node
 * \param row
 * \param column
 * \param parent
 * \return
 */
QModelIndex FeatureTreeViewModel::index(int row, int column,
                                        const QModelIndex& parent) const {
  if (!hasIndex(row, column, parent)) return QModelIndex();

  FeatureTreeItem* parentItem = nullptr;

  if (!parent.isValid())
    parentItem = rootItem;
  else
    parentItem = static_cast<FeatureTreeItem*>(parent.internalPointer());

  if (parentItem != nullptr) {
    FeatureTreeItem* childItem = parentItem->getChild(row);
    if (childItem != nullptr)
      return createIndex(row, column, childItem);
    else
      return QModelIndex();
  } else {
    return QModelIndex();
  }
}

/*!
 * \brief FeatureTreeViewModel::parent
 * \param index
 * \return
 */
QModelIndex FeatureTreeViewModel::parent(const QModelIndex& index) const {
  if (!index.isValid()) return QModelIndex();

  FeatureTreeItem* childItem =
      static_cast<FeatureTreeItem*>(index.internalPointer());
  if (childItem != nullptr) {
    FeatureTreeItem* parentItem = childItem->getParent();
    if (parentItem == nullptr || parentItem == rootItem) return QModelIndex();
    return createIndex(parentItem->getIndex(), 0, parentItem);
  } else {
    return QModelIndex();
  }
}

/*!
 * \brief FeatureTreeViewModel::getCurrentJob
 * \return
 */
const oi::Guarded<OiJob>& FeatureTreeViewModel::getCurrentJob() const {
  return this->currentJob;
}

/*!
 * \brief FeatureTreeViewModel::setCurrentJob
 * \param job
 */
void FeatureTreeViewModel::setCurrentJob(const oi::Guarded<OiJob>& job) {
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
 * \brief FeatureTreeViewModel::getRootItem
 * \return
 */
const QPointer<FeatureTreeItem>& FeatureTreeViewModel::getRootItem() const {
  return this->rootItem;
}

/*!
 * \brief FeatureTreeViewModel::updateModel
 */
void FeatureTreeViewModel::updateModel() {
  // check if a job is set and the root (header) item exists
  if (this->currentJob.isNull() || this->rootItem.isNull()) {
    return;
  }

  // start resetting the model
  emit this->beginResetModel();

  // delete the old tree hierarchy
  this->rootItem->deleteChildren();

  // add all stations
  if (this->currentJob->getStationsList().size() > 0) {
    FeatureTreeItem* item = new FeatureTreeItem(QString("%1:").arg(
        getElementTypePluralName(ElementTypes::eStationElement)));
    foreach (const oi::Guarded<Station>& station,
             this->currentJob->getStationsList()) {
      if (!station.isNull() && !station->getFeatureWrapper().isNull()) {
        FeatureTreeItem* itemStation =
            new FeatureTreeItem(station->getFeatureWrapper());
        item->appendChild(itemStation);
      }
    }
    this->rootItem->appendChild(item);
  }

  // add all nominal systems
  if (this->currentJob->getCoordinateSystemsList().size() > 0) {
    FeatureTreeItem* item = new FeatureTreeItem(QString("%1:").arg(
        getElementTypePluralName(ElementTypes::eCoordinateSystemElement)));
    foreach (const oi::Guarded<CoordinateSystem>& system,
             this->currentJob->getCoordinateSystemsList()) {
      if (!system.isNull() && !system->getFeatureWrapper().isNull()) {
        FeatureTreeItem* itemSystem =
            new FeatureTreeItem(system->getFeatureWrapper());
        item->appendChild(itemSystem);
      }
    }
    this->rootItem->appendChild(item);
  }

  // add all trafo params
  if (this->currentJob->getTransformationParametersList().size() > 0) {
    FeatureTreeItem* item = new FeatureTreeItem(QString("%1:").arg(
        getElementTypePluralName(ElementTypes::eTrafoParamElement)));
    foreach (const oi::Guarded<TrafoParam>& trafoParam,
             this->currentJob->getTransformationParametersList()) {
      if (!trafoParam.isNull() && !trafoParam->getFeatureWrapper().isNull()) {
        FeatureTreeItem* itemTrafoParam =
            new FeatureTreeItem(trafoParam->getFeatureWrapper());
        item->appendChild(itemTrafoParam);
      }
    }
    this->rootItem->appendChild(item);
  }

  // add all geometries
  this->addGeometries();

  // reset finished
  emit this->endResetModel();

  emit this->layoutAboutToBeChanged();
  emit this->layoutChanged();
}

/*!
 * \brief FeatureTreeViewModel::connectJob
 */
void FeatureTreeViewModel::connectJob() {
  QObject::connect(QtJobAdapter::of(this->currentJob.data()),
                   &QtJobAdapter::featureSetChanged, this,
                   &FeatureTreeViewModel::updateModel, Qt::AutoConnection);
  QObject::connect(QtJobAdapter::of(this->currentJob.data()),
                   &QtJobAdapter::featureNameChanged, this,
                   &FeatureTreeViewModel::updateModel, Qt::AutoConnection);
  QObject::connect(QtJobAdapter::of(this->currentJob.data()),
                   &QtJobAdapter::geometryObservationsChanged, this,
                   &FeatureTreeViewModel::updateModel, Qt::AutoConnection);
  QObject::connect(QtJobAdapter::of(this->currentJob.data()),
                   &QtJobAdapter::geometryNominalSystemChanged, this,
                   &FeatureTreeViewModel::updateModel, Qt::AutoConnection);
}

/*!
 * \brief FeatureTreeViewModel::disconnectJob
 */
void FeatureTreeViewModel::disconnectJob() {
  QObject::disconnect(QtJobAdapter::of(this->currentJob.data()),
                      &QtJobAdapter::featureSetChanged, this,
                      &FeatureTreeViewModel::updateModel);
  QObject::disconnect(QtJobAdapter::of(this->currentJob.data()),
                      &QtJobAdapter::featureNameChanged, this,
                      &FeatureTreeViewModel::updateModel);
  QObject::disconnect(QtJobAdapter::of(this->currentJob.data()),
                      &QtJobAdapter::geometryObservationsChanged, this,
                      &FeatureTreeViewModel::updateModel);
  QObject::disconnect(QtJobAdapter::of(this->currentJob.data()),
                      &QtJobAdapter::geometryNominalSystemChanged, this,
                      &FeatureTreeViewModel::updateModel);
}

/*!
 * \brief FeatureTreeViewModel::addGeometries
 */
void FeatureTreeViewModel::addGeometries() {
  // add all circles
  if (this->currentJob->getFeaturesByType(FeatureTypes::eCircleFeature).size() >
      0) {
    FeatureTreeItem* item = new FeatureTreeItem(QString("%1:").arg(
        getElementTypePluralName(ElementTypes::eCircleElement)));
    foreach (
        const oi::Guarded<FeatureWrapper>& feature,
        this->currentJob->getFeaturesByType(FeatureTypes::eCircleFeature)) {
      if (!feature.isNull() && !feature->getCircle().isNull()) {
        FeatureTreeItem* itemCircle = new FeatureTreeItem(feature);
        item->appendChild(itemCircle);
      }
    }
    this->rootItem->appendChild(item);
  }

  // add all cones
  if (this->currentJob->getFeaturesByType(FeatureTypes::eConeFeature).size() >
      0) {
    FeatureTreeItem* item = new FeatureTreeItem(QString("%1:").arg(
        getElementTypePluralName(ElementTypes::eConeElement)));
    foreach (const oi::Guarded<FeatureWrapper>& feature,
             this->currentJob->getFeaturesByType(FeatureTypes::eConeFeature)) {
      if (!feature.isNull() && !feature->getCone().isNull()) {
        FeatureTreeItem* itemCone = new FeatureTreeItem(feature);
        item->appendChild(itemCone);
      }
    }
    this->rootItem->appendChild(item);
  }

  // add all cylinders
  if (this->currentJob->getFeaturesByType(FeatureTypes::eCylinderFeature)
          .size() > 0) {
    FeatureTreeItem* item = new FeatureTreeItem(QString("%1:").arg(
        getElementTypePluralName(ElementTypes::eCylinderElement)));
    foreach (
        const oi::Guarded<FeatureWrapper>& feature,
        this->currentJob->getFeaturesByType(FeatureTypes::eCylinderFeature)) {
      if (!feature.isNull() && !feature->getCylinder().isNull()) {
        FeatureTreeItem* itemCylinder = new FeatureTreeItem(feature);
        item->appendChild(itemCylinder);
      }
    }
    this->rootItem->appendChild(item);
  }

  // add all ellipses
  if (this->currentJob->getFeaturesByType(FeatureTypes::eEllipseFeature)
          .size() > 0) {
    FeatureTreeItem* item = new FeatureTreeItem(QString("%1:").arg(
        getElementTypePluralName(ElementTypes::eEllipseElement)));
    foreach (
        const oi::Guarded<FeatureWrapper>& feature,
        this->currentJob->getFeaturesByType(FeatureTypes::eEllipseFeature)) {
      if (!feature.isNull() && !feature->getEllipse().isNull()) {
        FeatureTreeItem* itemEllipse = new FeatureTreeItem(feature);
        item->appendChild(itemEllipse);
      }
    }
    this->rootItem->appendChild(item);
  }

  // add all ellipsoids
  if (this->currentJob->getFeaturesByType(FeatureTypes::eEllipsoidFeature)
          .size() > 0) {
    FeatureTreeItem* item = new FeatureTreeItem(QString("%1:").arg(
        getElementTypePluralName(ElementTypes::eEllipsoidElement)));
    foreach (
        const oi::Guarded<FeatureWrapper>& feature,
        this->currentJob->getFeaturesByType(FeatureTypes::eEllipsoidFeature)) {
      if (!feature.isNull() && !feature->getEllipsoid().isNull()) {
        FeatureTreeItem* itemEllipsoid = new FeatureTreeItem(feature);
        item->appendChild(itemEllipsoid);
      }
    }
    this->rootItem->appendChild(item);
  }

  // add all hyperboloids
  if (this->currentJob->getFeaturesByType(FeatureTypes::eHyperboloidFeature)
          .size() > 0) {
    FeatureTreeItem* item = new FeatureTreeItem(QString("%1:").arg(
        getElementTypePluralName(ElementTypes::eHyperboloidElement)));
    foreach (const oi::Guarded<FeatureWrapper>& feature,
             this->currentJob->getFeaturesByType(
                 FeatureTypes::eHyperboloidFeature)) {
      if (!feature.isNull() && !feature->getHyperboloid().isNull()) {
        FeatureTreeItem* itemHyperboloid = new FeatureTreeItem(feature);
        item->appendChild(itemHyperboloid);
      }
    }
    this->rootItem->appendChild(item);
  }

  // add all lines
  if (this->currentJob->getFeaturesByType(FeatureTypes::eLineFeature).size() >
      0) {
    FeatureTreeItem* item = new FeatureTreeItem(QString("%1:").arg(
        getElementTypePluralName(ElementTypes::eLineElement)));
    foreach (const oi::Guarded<FeatureWrapper>& feature,
             this->currentJob->getFeaturesByType(FeatureTypes::eLineFeature)) {
      if (!feature.isNull() && !feature->getLine().isNull()) {
        FeatureTreeItem* itemLine = new FeatureTreeItem(feature);
        item->appendChild(itemLine);
      }
    }
    this->rootItem->appendChild(item);
  }

  // add all nurbs
  if (this->currentJob->getFeaturesByType(FeatureTypes::eNurbsFeature).size() >
      0) {
    FeatureTreeItem* item = new FeatureTreeItem(QString("%1:").arg(
        getElementTypePluralName(ElementTypes::eNurbsElement)));
    foreach (const oi::Guarded<FeatureWrapper>& feature,
             this->currentJob->getFeaturesByType(FeatureTypes::eNurbsFeature)) {
      if (!feature.isNull() && !feature->getNurbs().isNull()) {
        FeatureTreeItem* itemNurbs = new FeatureTreeItem(feature);
        item->appendChild(itemNurbs);
      }
    }
    this->rootItem->appendChild(item);
  }

  // add all paraboloids
  if (this->currentJob->getFeaturesByType(FeatureTypes::eParaboloidFeature)
          .size() > 0) {
    FeatureTreeItem* item = new FeatureTreeItem(QString("%1:").arg(
        getElementTypePluralName(ElementTypes::eParaboloidElement)));
    foreach (
        const oi::Guarded<FeatureWrapper>& feature,
        this->currentJob->getFeaturesByType(FeatureTypes::eParaboloidFeature)) {
      if (!feature.isNull() && !feature->getParaboloid().isNull()) {
        FeatureTreeItem* itemParaboloid = new FeatureTreeItem(feature);
        item->appendChild(itemParaboloid);
      }
    }
    this->rootItem->appendChild(item);
  }

  // add all planes
  if (this->currentJob->getFeaturesByType(FeatureTypes::ePlaneFeature).size() >
      0) {
    FeatureTreeItem* item = new FeatureTreeItem(QString("%1:").arg(
        getElementTypePluralName(ElementTypes::ePlaneElement)));
    foreach (const oi::Guarded<FeatureWrapper>& feature,
             this->currentJob->getFeaturesByType(FeatureTypes::ePlaneFeature)) {
      if (!feature.isNull() && !feature->getPlane().isNull()) {
        FeatureTreeItem* itemPlane = new FeatureTreeItem(feature);
        item->appendChild(itemPlane);
      }
    }
    this->rootItem->appendChild(item);
  }

  // add all points
  if (this->currentJob->getFeaturesByType(FeatureTypes::ePointFeature).size() >
      0) {
    FeatureTreeItem* item = new FeatureTreeItem(QString("%1:").arg(
        getElementTypePluralName(ElementTypes::ePointElement)));
    foreach (const oi::Guarded<FeatureWrapper>& feature,
             this->currentJob->getFeaturesByType(FeatureTypes::ePointFeature)) {
      if (!feature.isNull() && !feature->getPoint().isNull()) {
        FeatureTreeItem* itemPoint = new FeatureTreeItem(feature);
        item->appendChild(itemPoint);
      }
    }
    this->rootItem->appendChild(item);
  }

  // add all point clouds
  if (this->currentJob->getFeaturesByType(FeatureTypes::ePointCloudFeature)
          .size() > 0) {
    FeatureTreeItem* item = new FeatureTreeItem(QString("%1:").arg(
        getElementTypePluralName(ElementTypes::ePointCloudElement)));
    foreach (
        const oi::Guarded<FeatureWrapper>& feature,
        this->currentJob->getFeaturesByType(FeatureTypes::ePointCloudFeature)) {
      if (!feature.isNull() && !feature->getPointCloud().isNull()) {
        FeatureTreeItem* itemCloud = new FeatureTreeItem(feature);
        item->appendChild(itemCloud);
      }
    }
    this->rootItem->appendChild(item);
  }

  // add all angles
  if (this->currentJob
          ->getFeaturesByType(FeatureTypes::eScalarEntityAngleFeature)
          .size() > 0) {
    FeatureTreeItem* item = new FeatureTreeItem(QString("%1:").arg(
        getElementTypePluralName(ElementTypes::eScalarEntityAngleElement)));
    foreach (const oi::Guarded<FeatureWrapper>& feature,
             this->currentJob->getFeaturesByType(
                 FeatureTypes::eScalarEntityAngleFeature)) {
      if (!feature.isNull() && !feature->getScalarEntityAngle().isNull()) {
        FeatureTreeItem* itemAngle = new FeatureTreeItem(feature);
        item->appendChild(itemAngle);
      }
    }
    this->rootItem->appendChild(item);
  }

  // add all distances
  if (this->currentJob
          ->getFeaturesByType(FeatureTypes::eScalarEntityDistanceFeature)
          .size() > 0) {
    FeatureTreeItem* item = new FeatureTreeItem(QString("%1:").arg(
        getElementTypePluralName(ElementTypes::eScalarEntityDistanceElement)));
    foreach (const oi::Guarded<FeatureWrapper>& feature,
             this->currentJob->getFeaturesByType(
                 FeatureTypes::eScalarEntityDistanceFeature)) {
      if (!feature.isNull() && !feature->getScalarEntityDistance().isNull()) {
        FeatureTreeItem* itemDistance = new FeatureTreeItem(feature);
        item->appendChild(itemDistance);
      }
    }
    this->rootItem->appendChild(item);
  }

  // add all measurement series
  if (this->currentJob
          ->getFeaturesByType(
              FeatureTypes::eScalarEntityMeasurementSeriesFeature)
          .size() > 0) {
    FeatureTreeItem* item =
        new FeatureTreeItem(QString("%1:").arg(getElementTypePluralName(
            ElementTypes::eScalarEntityMeasurementSeriesElement)));
    foreach (const oi::Guarded<FeatureWrapper>& feature,
             this->currentJob->getFeaturesByType(
                 FeatureTypes::eScalarEntityMeasurementSeriesFeature)) {
      if (!feature.isNull() &&
          !feature->getScalarEntityMeasurementSeries().isNull()) {
        FeatureTreeItem* itemSeries = new FeatureTreeItem(feature);
        item->appendChild(itemSeries);
      }
    }
    this->rootItem->appendChild(item);
  }

  // add all temperatures
  if (this->currentJob
          ->getFeaturesByType(FeatureTypes::eScalarEntityTemperatureFeature)
          .size() > 0) {
    FeatureTreeItem* item =
        new FeatureTreeItem(QString("%1:").arg(getElementTypePluralName(
            ElementTypes::eScalarEntityTemperatureElement)));
    foreach (const oi::Guarded<FeatureWrapper>& feature,
             this->currentJob->getFeaturesByType(
                 FeatureTypes::eScalarEntityTemperatureFeature)) {
      if (!feature.isNull() &&
          !feature->getScalarEntityTemperature().isNull()) {
        FeatureTreeItem* itemTemperature = new FeatureTreeItem(feature);
        item->appendChild(itemTemperature);
      }
    }
    this->rootItem->appendChild(item);
  }

  // add all slotted holes
  if (this->currentJob->getFeaturesByType(FeatureTypes::eSlottedHoleFeature)
          .size() > 0) {
    FeatureTreeItem* item = new FeatureTreeItem(QString("%1:").arg(
        getElementTypePluralName(ElementTypes::eSlottedHoleElement)));
    foreach (const oi::Guarded<FeatureWrapper>& feature,
             this->currentJob->getFeaturesByType(
                 FeatureTypes::eSlottedHoleFeature)) {
      if (!feature.isNull() && !feature->getSlottedHole().isNull()) {
        FeatureTreeItem* itemSlottedHole = new FeatureTreeItem(feature);
        item->appendChild(itemSlottedHole);
      }
    }
    this->rootItem->appendChild(item);
  }

  // add all spheres
  if (this->currentJob->getFeaturesByType(FeatureTypes::eSphereFeature).size() >
      0) {
    FeatureTreeItem* item = new FeatureTreeItem(QString("%1:").arg(
        getElementTypePluralName(ElementTypes::eSphereElement)));
    foreach (
        const oi::Guarded<FeatureWrapper>& feature,
        this->currentJob->getFeaturesByType(FeatureTypes::eSphereFeature)) {
      if (!feature.isNull() && !feature->getSphere().isNull()) {
        FeatureTreeItem* itemSphere = new FeatureTreeItem(feature);
        item->appendChild(itemSphere);
      }
    }
    this->rootItem->appendChild(item);
  }

  // add all tori
  if (this->currentJob->getFeaturesByType(FeatureTypes::eTorusFeature).size() >
      0) {
    FeatureTreeItem* item = new FeatureTreeItem(QString("%1:").arg(
        getElementTypePluralName(ElementTypes::eTorusElement)));
    foreach (const oi::Guarded<FeatureWrapper>& feature,
             this->currentJob->getFeaturesByType(FeatureTypes::eTorusFeature)) {
      if (!feature.isNull() && !feature->getTorus().isNull()) {
        FeatureTreeItem* itemTorus = new FeatureTreeItem(feature);
        item->appendChild(itemTorus);
      }
    }
    this->rootItem->appendChild(item);
  }
}

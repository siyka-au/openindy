#include "reading_model.h"

#include <utility>

#include "qt_convert.h"
#include "qt_job_adapter.h"

namespace {
constexpr int kDisplayDigits = 6;
}  // namespace

/*!
 * \brief ReadingModel::ReadingModel
 * \param parent
 */
ReadingModel::ReadingModel(QObject* parent) : QAbstractTableModel(parent) {}

/*!
 * \brief ReadingModel::rowCount
 * \param parent
 * \return
 */
int ReadingModel::rowCount(const QModelIndex& parent) const {
  // check current job
  if (this->currentJob.isNull()) {
    return 0;
  }

  // get and check active feature
  oi::Guarded<FeatureWrapper> feature = this->currentJob->getActiveFeature();
  if (feature.isNull() || feature->getFeature().isNull()) {
    return 0;
  }

  // get and check active geometry
  oi::Guarded<Geometry> geometry(nullptr);
  if (getIsGeometry(feature->getFeatureTypeEnum())) {
    geometry = feature->getGeometry();
  } else if (feature->getFeatureTypeEnum() == FeatureTypes::eStationFeature &&
             !feature->getStation().isNull()) {
    geometry = feature->getStation()->getPosition();
  }
  if (geometry.isNull()) {
    return 0;
  }

  return geometry->getObservations().size();
}

/*!
 * \brief ReadingModel::columnCount
 * \param parent
 * \return
 */
int ReadingModel::columnCount(const QModelIndex& parent) const {
  return getReadingDisplayAttributes().size();
}

/*!
 * \brief ReadingModel::data
 * \param index
 * \param role
 * \return
 */
QVariant ReadingModel::data(const QModelIndex& index, int role) const {
  // check role
  if (role != Qt::DisplayRole && role != Qt::TextAlignmentRole) {
    return QVariant();
  }

  // check current job and model index
  if (this->currentJob.isNull() || !index.isValid()) {
    return QVariant();
  }

  // get and check active feature
  oi::Guarded<FeatureWrapper> feature = this->currentJob->getActiveFeature();
  if (feature.isNull() || feature->getFeature().isNull()) {
    return QVariant();
  }

  // get and check active geometry
  oi::Guarded<Geometry> geometry(nullptr);
  if (getIsGeometry(feature->getFeatureTypeEnum())) {
    geometry = feature->getGeometry();
  } else if (feature->getFeatureTypeEnum() == FeatureTypes::eStationFeature &&
             !feature->getStation().isNull()) {
    geometry = feature->getStation()->getPosition();
  }
  if (geometry.isNull()) {
    return QVariant();
  }

  // get row and column indices
  int rowIndex = index.row();
  int columnIndex = index.column();

  // get and check reading
  oi::Guarded<Reading> reading(nullptr);
  if (std::cmp_less(rowIndex, geometry->getObservations().size()) &&
      !geometry->getObservations().at(rowIndex).isNull()) {
    reading = geometry->getObservations().at(rowIndex)->getReading();
  }
  if (reading.isNull()) {
    return QVariant();
  }

  if (role == Qt::DisplayRole) {
    switch ((ReadingDisplayAttributes)columnIndex) {
      case eReadingDisplayId:
        return toQString(reading->getDisplayId());
      case eReadingDisplayType:
        return toQString(reading->getDisplayType());
      case eReadingDisplayTime:
        return toQString(reading->getDisplayTime());
      case eReadingDisplaySensorConfigName:
        return toQString(reading->getDisplaySensorConfigName());
      case eReadingDisplayMeasurementConfigName:
        return toQString(reading->getDisplayMeasurementConfigName());
      case eReadingDisplaySide:
        return toQString(reading->getDisplaySide());
      case eReadingDisplayAzimuth:
        return toQString(reading->getDisplayAzimuth(
            this->parameterDisplayConfig.getDisplayUnit(
                DimensionType::eAngular),
            this->parameterDisplayConfig.getDisplayDigits(
                DimensionType::eAngular)));
      case eReadingDisplayZenith:
        return toQString(reading->getDisplayZenith(
            this->parameterDisplayConfig.getDisplayUnit(
                DimensionType::eAngular),
            this->parameterDisplayConfig.getDisplayDigits(
                DimensionType::eAngular)));
      case eReadingDisplayDistance:
        return toQString(reading->getDisplayDistance(
            this->parameterDisplayConfig.getDisplayUnit(DimensionType::eMetric),
            this->parameterDisplayConfig.getDisplayDigits(
                DimensionType::eMetric)));
      case eReadingDisplayX:
        return toQString(reading->getDisplayX(
            this->parameterDisplayConfig.getDisplayUnit(DimensionType::eMetric),
            this->parameterDisplayConfig.getDisplayDigits(
                DimensionType::eMetric)));
      case eReadingDisplayY:
        return toQString(reading->getDisplayY(
            this->parameterDisplayConfig.getDisplayUnit(DimensionType::eMetric),
            this->parameterDisplayConfig.getDisplayDigits(
                DimensionType::eMetric)));
      case eReadingDisplayZ:
        return toQString(reading->getDisplayZ(
            this->parameterDisplayConfig.getDisplayUnit(DimensionType::eMetric),
            this->parameterDisplayConfig.getDisplayDigits(
                DimensionType::eMetric)));
      case eReadingDisplayI:
        return toQString(reading->getDisplayI(kDisplayDigits));
      case eReadingDisplayJ:
        return toQString(reading->getDisplayJ(kDisplayDigits));
      case eReadingDisplayK:
        return toQString(reading->getDisplayK(kDisplayDigits));
      case eReadingDisplayTemperature:
        return toQString(reading->getDisplayTemperature(
            this->parameterDisplayConfig.getDisplayUnit(
                DimensionType::eTemperature),
            this->parameterDisplayConfig.getDisplayDigits(
                DimensionType::eTemperature)));
      case eReadingDisplayImported:
        return reading->isImported() ? "imported" : "";
      case eReadingDisplaySigmaAzimuth:
        return toQString(reading->getDisplaySigmaAzimuth(
            this->parameterDisplayConfig.getDisplayUnit(
                DimensionType::eAngular),
            this->parameterDisplayConfig.getDisplayDigits(
                DimensionType::eAngular)));
      case eReadingDisplaySigmaZenith:
        return toQString(reading->getDisplaySigmaZenith(
            this->parameterDisplayConfig.getDisplayUnit(
                DimensionType::eAngular),
            this->parameterDisplayConfig.getDisplayDigits(
                DimensionType::eAngular)));
      case eReadingDisplaySigmaDistance:
        return toQString(reading->getDisplaySigmaDistance(
            this->parameterDisplayConfig.getDisplayUnit(DimensionType::eMetric),
            this->parameterDisplayConfig.getDisplayDigits(
                DimensionType::eMetric)));
      case eReadingDisplaySigmaX:
        return toQString(reading->getDisplaySigmaX(
            this->parameterDisplayConfig.getDisplayUnit(DimensionType::eMetric),
            this->parameterDisplayConfig.getDisplayDigits(
                DimensionType::eMetric)));
      case eReadingDisplaySigmaY:
        return toQString(reading->getDisplaySigmaY(
            this->parameterDisplayConfig.getDisplayUnit(DimensionType::eMetric),
            this->parameterDisplayConfig.getDisplayDigits(
                DimensionType::eMetric)));
      case eReadingDisplaySigmaZ:
        return toQString(reading->getDisplaySigmaZ(
            this->parameterDisplayConfig.getDisplayUnit(DimensionType::eMetric),
            this->parameterDisplayConfig.getDisplayDigits(
                DimensionType::eMetric)));
      case eReadingDisplaySigmaI:
        return toQString(reading->getDisplaySigmaI(kDisplayDigits));
      case eReadingDisplaySigmaJ:
        return toQString(reading->getDisplaySigmaJ(kDisplayDigits));
      case eReadingDisplaySigmaK:
        return toQString(reading->getDisplaySigmaK(kDisplayDigits));
      case eReadingDisplaySigmaTemperature:
        return toQString(reading->getDisplaySigmaTemperature(
            this->parameterDisplayConfig.getDisplayUnit(
                DimensionType::eTemperature),
            this->parameterDisplayConfig.getDisplayDigits(
                DimensionType::eTemperature)));
    }

  } else if (role == Qt::TextAlignmentRole) {
    switch ((ReadingDisplayAttributes)columnIndex) {
      case eReadingDisplayAzimuth:
      case eReadingDisplayZenith:
      case eReadingDisplayDistance:
      case eReadingDisplayX:
      case eReadingDisplayY:
      case eReadingDisplayZ:
      case eReadingDisplayI:
      case eReadingDisplayJ:
      case eReadingDisplayK:
      case eReadingDisplayTemperature:
      case eReadingDisplaySigmaAzimuth:
      case eReadingDisplaySigmaZenith:
      case eReadingDisplaySigmaDistance:
      case eReadingDisplaySigmaX:
      case eReadingDisplaySigmaY:
      case eReadingDisplaySigmaZ:
      case eReadingDisplaySigmaI:
      case eReadingDisplaySigmaJ:
      case eReadingDisplaySigmaK:
      case eReadingDisplaySigmaTemperature:
        return QVariant(Qt::AlignRight | Qt::AlignVCenter);
    }
  }
  return QVariant();
}

/*!
 * \brief ReadingModel::headerData
 * \param section
 * \param orientation
 * \param role
 * \return
 */
QVariant ReadingModel::headerData(int section, Qt::Orientation orientation,
                                  int role) const {
  if ((Qt::DisplayRole == role) && (Qt::Horizontal == orientation) &&
      (0 <= section) && (section < columnCount())) {
    return toQString(getReadingDisplayAttributeName(
        getReadingDisplayAttributes().at(section)));
  }

  return QVariant();
}

/*!
 * \brief ReadingModel::flags
 * \param index
 * \return
 */
Qt::ItemFlags ReadingModel::flags(const QModelIndex& index) const {
  return Qt::NoItemFlags;
}

/*!
 * \brief ReadingModel::getCurrentJob
 * \return
 */
const oi::Guarded<OiJob>& ReadingModel::getCurrentJob() const {
  return this->currentJob;
}

/*!
 * \brief ReadingModel::setCurrentJob
 * \param job
 */
void ReadingModel::setCurrentJob(const oi::Guarded<OiJob>& job) {
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
 * \brief ReadingModel::getParameterDisplayConfig
 * \return
 */
const ParameterDisplayConfig& ReadingModel::getParameterDisplayConfig() const {
  return this->parameterDisplayConfig;
}

/*!
 * \brief ReadingModel::setParameterDisplayConfig
 * \param config
 */
void ReadingModel::setParameterDisplayConfig(
    const ParameterDisplayConfig& config) {
  this->parameterDisplayConfig = config;
  this->updateModel();
}

/*!
 * \brief ReadingModel::updateModel
 */
void ReadingModel::updateModel() {
  emit layoutAboutToBeChanged();
  emit layoutChanged();
}

void ReadingModel::geometryObservationsChanged(const int& featureId) {
  this->updateModel();
}

void ReadingModel::activeFeatureChanged() { this->updateModel(); }

/*!
 * \brief ReadingModel::connectJob
 */
void ReadingModel::connectJob() {
  // check current job
  if (this->currentJob.isNull()) {
    return;
  }

  QObject::connect(QtJobAdapter::of(this->currentJob.data()),
                   &QtJobAdapter::activeFeatureChanged, this,
                   &ReadingModel::activeFeatureChanged, Qt::AutoConnection);
  QObject::connect(QtJobAdapter::of(this->currentJob.data()),
                   &QtJobAdapter::geometryObservationsChanged, this,
                   &ReadingModel::geometryObservationsChanged,
                   Qt::AutoConnection);
}

/*!
 * \brief ReadingModel::disconnectJob
 */
void ReadingModel::disconnectJob() {
  // check current job
  if (this->currentJob.isNull()) {
    return;
  }

  QObject::disconnect(QtJobAdapter::of(this->currentJob.data()),
                      &QtJobAdapter::activeFeatureChanged, this,
                      &ReadingModel::activeFeatureChanged);
  QObject::disconnect(QtJobAdapter::of(this->currentJob.data()),
                      &QtJobAdapter::geometryObservationsChanged, this,
                      &ReadingModel::geometryObservationsChanged);
}

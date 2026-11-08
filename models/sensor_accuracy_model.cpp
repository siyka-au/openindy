#include "sensor_accuracy_model.h"

#include <utility>

#include "containers.h"
#include "qt_convert.h"

namespace {
constexpr int kDirectionDigits = 6;  // decimals of unitless sigma values
}  // namespace

using namespace oi;

/*!
 * \brief SensorAccuracyModel::SensorAccuracyModel
 * \param parent
 */
SensorAccuracyModel::SensorAccuracyModel(QObject* parent)
    : QAbstractTableModel(parent) {}

/*!
 * \brief SensorAccuracyModel::rowCount
 * \param parent
 * \return
 */
int SensorAccuracyModel::rowCount(const QModelIndex& parent) const {
  if (this->sConfig.getIsValid() &&
      this->sConfig.getPluginName().compare("") != 0 &&
      this->sConfig.getSensorName().compare("") != 0) {
    return (SensorAccuracyRows::kRowFirstUndefined +
            oi::values(this->sConfig.getAccuracy().sigmaUndefined).size());
  }
  return 0;
}

/*!
 * \brief SensorAccuracyModel::columnCount
 * \param parent
 * \return
 */
int SensorAccuracyModel::columnCount(const QModelIndex& parent) const {
  return 1;
}

/*!
 * \brief SensorAccuracyModel::data
 * \param index
 * \param role
 * \return
 */
QVariant SensorAccuracyModel::data(const QModelIndex& index, int role) const {
  // check model index
  if (!index.isValid()) {
    return QVariant();
  }

  // get row and column indices
  int rowIndex = index.row();
  int columnIndex = index.column();

  if (role == Qt::DisplayRole) {
    // sigma type headers
    switch (rowIndex) {
      case SensorAccuracyRows::kRowPolarHeader:
        return "polar";
      case SensorAccuracyRows::kRowCartesianHeader:
        return "cartesian";
      case SensorAccuracyRows::kRowLevelHeader:
        return "level";
      case SensorAccuracyRows::kRowTemperatureHeader:
        return "temperature";
      case SensorAccuracyRows::kRowUndefinedHeader:
        return "undefined";
    }

    // accuracy values
    switch (rowIndex) {
      case SensorAccuracyRows::kRowAzimuth:
        return QString::number(
            convertFromDefault(this->sConfig.getAccuracy().sigmaAzimuth,
                               this->parameterDisplayConfig.getDisplayUnit(
                                   DimensionType::eAngular)),
            'f',
            this->parameterDisplayConfig.getDisplayDigits(
                DimensionType::eAngular));
      case SensorAccuracyRows::kRowZenith:
        return QString::number(
            convertFromDefault(this->sConfig.getAccuracy().sigmaZenith,
                               this->parameterDisplayConfig.getDisplayUnit(
                                   DimensionType::eAngular)),
            'f',
            this->parameterDisplayConfig.getDisplayDigits(
                DimensionType::eAngular));
      case SensorAccuracyRows::kRowDistance:
        return QString::number(
            convertFromDefault(this->sConfig.getAccuracy().sigmaDistance,
                               this->parameterDisplayConfig.getDisplayUnit(
                                   DimensionType::eMetric)),
            'f',
            this->parameterDisplayConfig.getDisplayDigits(
                DimensionType::eMetric));
      case SensorAccuracyRows::kRowX:
        return QString::number(
            convertFromDefault(this->sConfig.getAccuracy().sigmaXyz.getAt(0),
                               this->parameterDisplayConfig.getDisplayUnit(
                                   DimensionType::eMetric)),
            'f',
            this->parameterDisplayConfig.getDisplayDigits(
                DimensionType::eMetric));
      case SensorAccuracyRows::kRowY:
        return QString::number(
            convertFromDefault(this->sConfig.getAccuracy().sigmaXyz.getAt(1),
                               this->parameterDisplayConfig.getDisplayUnit(
                                   DimensionType::eMetric)),
            'f',
            this->parameterDisplayConfig.getDisplayDigits(
                DimensionType::eMetric));
      case SensorAccuracyRows::kRowZ:
        return QString::number(
            convertFromDefault(this->sConfig.getAccuracy().sigmaXyz.getAt(2),
                               this->parameterDisplayConfig.getDisplayUnit(
                                   DimensionType::eMetric)),
            'f',
            this->parameterDisplayConfig.getDisplayDigits(
                DimensionType::eMetric));
      case SensorAccuracyRows::kRowI:
        return QString::number(this->sConfig.getAccuracy().sigmaI, 'f',
                               kDirectionDigits);
      case SensorAccuracyRows::kRowJ:
        return QString::number(this->sConfig.getAccuracy().sigmaJ, 'f',
                               kDirectionDigits);
      case SensorAccuracyRows::kRowK:
        return QString::number(this->sConfig.getAccuracy().sigmaK, 'f',
                               kDirectionDigits);
      case SensorAccuracyRows::kRowTemperature:
        return QString::number(
            convertFromDefault(this->sConfig.getAccuracy().sigmaTemp,
                               this->parameterDisplayConfig.getDisplayUnit(
                                   DimensionType::eTemperature)),
            'f',
            this->parameterDisplayConfig.getDisplayDigits(
                DimensionType::eTemperature));
      default:
        int index = rowIndex - SensorAccuracyRows::kRowFirstUndefined;
        if (index >= 0 &&
            std::cmp_less(index,
                          this->sConfig.getAccuracy().sigmaUndefined.size())) {
          return QString::number(
              oi::values(this->sConfig.getAccuracy().sigmaUndefined).at(index),
              'f', kDirectionDigits);
        }
    }

  } else if (role == Qt::BackgroundRole) {
    // sigma type headers
    if (rowIndex == SensorAccuracyRows::kRowPolarHeader ||
        rowIndex == SensorAccuracyRows::kRowCartesianHeader ||
        rowIndex == SensorAccuracyRows::kRowLevelHeader ||
        rowIndex == SensorAccuracyRows::kRowTemperatureHeader ||
        rowIndex == SensorAccuracyRows::kRowUndefinedHeader) {
      return QColor(Qt::lightGray);
    }

  } else if (role == Qt::ForegroundRole) {
    // sigma type headers: use a readable text color against the custom
    // background above
    if (rowIndex == SensorAccuracyRows::kRowPolarHeader ||
        rowIndex == SensorAccuracyRows::kRowCartesianHeader ||
        rowIndex == SensorAccuracyRows::kRowLevelHeader ||
        rowIndex == SensorAccuracyRows::kRowTemperatureHeader ||
        rowIndex == SensorAccuracyRows::kRowUndefinedHeader) {
      return QColor(Qt::black);
    }
  }

  return QVariant();
}

/*!
 * \brief SensorAccuracyModel::headerData
 * \param section
 * \param orientation
 * \param role
 * \return
 */
QVariant SensorAccuracyModel::headerData(int section,
                                         Qt::Orientation orientation,
                                         int role) const {
  if ((Qt::DisplayRole == role) && (Qt::Vertical == orientation) &&
      (0 <= section) && (section < rowCount())) {
    switch (section) {
      case SensorAccuracyRows::kRowAzimuth:
        return "azimuth";
      case SensorAccuracyRows::kRowZenith:
        return "zenith";
      case SensorAccuracyRows::kRowDistance:
        return "distance";
      case SensorAccuracyRows::kRowX:
        return "x";
      case SensorAccuracyRows::kRowY:
        return "y";
      case SensorAccuracyRows::kRowZ:
        return "z";
      case SensorAccuracyRows::kRowI:
        return "i";
      case SensorAccuracyRows::kRowJ:
        return "j";
      case SensorAccuracyRows::kRowK:
        return "k";
      case SensorAccuracyRows::kRowTemperature:
        return "temperature";
      default:
        int index = section - SensorAccuracyRows::kRowFirstUndefined;
        if (index >= 0 &&
            std::cmp_less(index,
                          this->sConfig.getAccuracy().sigmaUndefined.size())) {
          return toQString(
              oi::keys(this->sConfig.getAccuracy().sigmaUndefined).at(index));
        }
    }

  } else if ((Qt::DisplayRole == role) && (Qt::Horizontal == orientation) &&
             (0 <= section) && (section < columnCount())) {
    switch (section) {
      case SensorAccuracyRows::kRowPolarHeader:
        return QString("sigma - %1 %2 %3")
            .arg(getUnitTypeName(this->parameterDisplayConfig.getDisplayUnit(
                DimensionType::eMetric)))
            .arg(getUnitTypeName(this->parameterDisplayConfig.getDisplayUnit(
                DimensionType::eAngular)))
            .arg(getUnitTypeName(this->parameterDisplayConfig.getDisplayUnit(
                DimensionType::eTemperature)));
    }
  }

  return QVariant();
}

/*!
 * \brief SensorAccuracyModel::flags
 * Add edit-functionality to some cells of tablemodel
 * \param index
 * \return
 */
Qt::ItemFlags SensorAccuracyModel::flags(const QModelIndex& index) const {
  Qt::ItemFlags myFlags = QAbstractTableModel::flags(index);

  // check read only state
  if (this->isReadOnly) {
    return myFlags;
  }

  // check sensor config
  if (!this->sConfig.getIsValid()) {
    return myFlags;
  }

  // check model index
  if (!index.isValid()) {
    return myFlags;
  }

  // get row and column indices
  int rowIndex = index.row();
  int columnIndex = index.column();

  if (rowIndex != SensorAccuracyRows::kRowPolarHeader &&
      rowIndex != SensorAccuracyRows::kRowCartesianHeader &&
      rowIndex != SensorAccuracyRows::kRowLevelHeader &&
      rowIndex != SensorAccuracyRows::kRowTemperatureHeader &&
      rowIndex != SensorAccuracyRows::kRowUndefinedHeader) {
    return (myFlags | Qt::ItemIsEditable);
  }
  return myFlags;
}

/*!
 * \brief SensorAccuracyModel::setData
 * Update the edited accuracy parameters
 * \param index
 * \param value
 * \param role
 * \return
 */
bool SensorAccuracyModel::setData(const QModelIndex& index,
                                  const QVariant& value, int role) {
  // check model index
  if (!index.isValid()) {
    return false;
  }

  // check read only state
  if (this->isReadOnly) {
    return false;
  }

  // check sensor config
  if (!this->sConfig.getIsValid()) {
    return false;
  }

  // get row and column indices
  int rowIndex = index.row();
  int columnIndex = index.column();

  if (rowIndex < rowCount()) {
    Accuracy accuracy = this->sConfig.getAccuracy();

    switch (rowIndex) {
      case SensorAccuracyRows::kRowAzimuth:
        accuracy.sigmaAzimuth = convertToDefault(
            value.toDouble(), this->parameterDisplayConfig.getDisplayUnit(
                                  DimensionType::eAngular));
        break;
      case SensorAccuracyRows::kRowZenith:
        accuracy.sigmaZenith = convertToDefault(
            value.toDouble(), this->parameterDisplayConfig.getDisplayUnit(
                                  DimensionType::eAngular));
        break;
      case SensorAccuracyRows::kRowDistance:
        accuracy.sigmaDistance = convertToDefault(
            value.toDouble(), this->parameterDisplayConfig.getDisplayUnit(
                                  DimensionType::eMetric));
        break;
      case SensorAccuracyRows::kRowX:
        accuracy.sigmaXyz.setAt(
            0, convertToDefault(value.toDouble(),
                                this->parameterDisplayConfig.getDisplayUnit(
                                    DimensionType::eMetric)));
        break;
      case SensorAccuracyRows::kRowY:
        accuracy.sigmaXyz.setAt(
            1, convertToDefault(value.toDouble(),
                                this->parameterDisplayConfig.getDisplayUnit(
                                    DimensionType::eMetric)));
        break;
      case SensorAccuracyRows::kRowZ:
        accuracy.sigmaXyz.setAt(
            2, convertToDefault(value.toDouble(),
                                this->parameterDisplayConfig.getDisplayUnit(
                                    DimensionType::eMetric)));
        break;
      case SensorAccuracyRows::kRowI:
        accuracy.sigmaI = value.toDouble();
        break;
      case SensorAccuracyRows::kRowJ:
        accuracy.sigmaJ = value.toDouble();
        break;
      case SensorAccuracyRows::kRowK:
        accuracy.sigmaK = value.toDouble();
        break;
      case SensorAccuracyRows::kRowTemperature:
        accuracy.sigmaTemp = convertToDefault(
            value.toDouble(), this->parameterDisplayConfig.getDisplayUnit(
                                  DimensionType::eTemperature));
        break;
      default:
        int index = rowIndex - SensorAccuracyRows::kRowFirstUndefined;
        if (index >= 0 &&
            std::cmp_less(index,
                          this->sConfig.getAccuracy().sigmaUndefined.size())) {
          accuracy.sigmaUndefined.insert_or_assign(
              oi::keys(this->sConfig.getAccuracy().sigmaUndefined).at(index),
              value.toDouble());
        }
        break;
    }

    // set new accuracy
    this->sConfig.setAccuracy(accuracy);

    emit this->sensorConfigurationChanged(this->sConfig);

    this->updateModel();

    return true;
  }

  return false;
}

/*!
 * \brief SensorAccuracyModel::getParameterDisplayConfig
 * \return
 */
const ParameterDisplayConfig& SensorAccuracyModel::getParameterDisplayConfig()
    const {
  return this->parameterDisplayConfig;
}

/*!
 * \brief SensorAccuracyModel::setParameterDisplayConfig
 * \param config
 */
void SensorAccuracyModel::setParameterDisplayConfig(
    const ParameterDisplayConfig& config) {
  this->parameterDisplayConfig = config;
  this->updateModel();
}

/*!
 * \brief SensorAccuracyModel::getSensorConfiguration
 * \return
 */
const SensorConfiguration& SensorAccuracyModel::getSensorConfiguration() const {
  return this->sConfig;
}

/*!
 * \brief SensorAccuracyModel::setSensorConfiguration
 * \param sConfig
 */
void SensorAccuracyModel::setSensorConfiguration(
    const SensorConfiguration& sConfig) {
  this->sConfig = sConfig;
  this->updateModel();
}

/*!
 * \brief SensorAccuracyModel::getIsReadOnly
 * \return
 */
bool SensorAccuracyModel::getIsReadOnly() const { return this->isReadOnly; }

/*!
 * \brief SensorAccuracyModel::setIsReadOnly
 * \param isReadOnly
 */
void SensorAccuracyModel::setIsReadOnly(bool isReadOnly) {
  this->isReadOnly = isReadOnly;
  this->updateModel();
}

/*!
 * \brief SensorAccuracyModel::updateModel
 */
void SensorAccuracyModel::updateModel() {
  emit this->layoutAboutToBeChanged();
  emit this->layoutChanged();
}

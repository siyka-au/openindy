#ifndef SENSORCONFIGURATIONMODEL_H
#define SENSORCONFIGURATIONMODEL_H

#include <QAbstractListModel>
#include <QModelIndex>
#include <QPixmap>
#include <QPointer>

#include "sensorconfigurationmanager.h"
#include "types.h"
#include "util.h"

/*!
 * \brief The SensorConfigurationTreeViewModel class
 * Model that holds all available sensor configurations
 */
class SensorConfigurationModel : public QAbstractListModel {
  Q_OBJECT

 public:
  explicit SensorConfigurationModel(QObject* parent = nullptr);

  // #######################################
  // override methods of abstract item model
  // #######################################

  [[nodiscard]] int rowCount(
      const QModelIndex& parent = QModelIndex()) const override;
  [[nodiscard]] int columnCount(
      const QModelIndex& parent = QModelIndex()) const override;

  [[nodiscard]] QVariant data(const QModelIndex& index,
                              int role = Qt::DisplayRole) const override;

  [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation,
                                    int role) const override;

  [[nodiscard]] Qt::ItemFlags flags(const QModelIndex& index) const override;

  bool setData(const QModelIndex& index, const QVariant& value,
               int role = Qt::EditRole) override;

  // #######################################
  // get or set sensor configuration manager
  // #######################################

  [[nodiscard]] const SensorConfigurationManager&
  getSensorConfigurationManager() const;
  void setSensorConfigurationManager(const SensorConfigurationManager& manager);

  // ####################################
  // get or set sensor config information
  // ####################################

  // add or remove configs
  SensorConfiguration addSensorConfiguration();
  bool removeSensorConfiguration(const QModelIndex& index);

  // update the active sensor config
  void updateSensorAccuracy(const SensorConfiguration& sConfig);
  void updateSensorParameters(const SensorConfiguration& sConfig);
  void updateSensorPlugin(const SensorConfiguration& sConfig);
  void updateSensorConnection(const SensorConfiguration& sConfig);

  // get or set active sensor config
  [[nodiscard]] const SensorConfiguration& getActiveSensorConfig() const;
  [[nodiscard]] QModelIndex getActiveSensorConfigIndex() const;
  void setActiveSensorConfig(const SensorConfiguration& sConfig);
  void setActiveSensorConfig(const QModelIndex& index);

 signals:

  // ##################################
  // inform about sensor config changes
  // ##################################

  // active config changed
  void activeSensorConfigChanged();

  // config renamed
  void sensorConfigRenamed();

  // configs added or removed
  void sensorConfigsChanged();

 public slots:

  // ###########################################
  // update the model when configuration changes
  // ###########################################

  void updateModel();

 private:
  // #####################
  // sensor config manager
  // #####################

  SensorConfigurationManager sensorConfigManager;
};

#endif  // SENSORCONFIGURATIONMODEL_H

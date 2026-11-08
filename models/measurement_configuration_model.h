#ifndef MEASUREMENTCONFIGURATIONMODEL_H
#define MEASUREMENTCONFIGURATIONMODEL_H

#include <QAbstractListModel>
#include <QModelIndex>
#include <QPixmap>
#include <QPointer>

#include "measurement_config_manager.h"
#include "types.h"
#include "util.h"

#if defined(OI_MAIN_LIB)
#define OI_MAIN_EXPORT Q_DECL_EXPORT
#else
#define OI_MAIN_EXPORT Q_DECL_IMPORT
#endif

using namespace oi;

/*!
 * \brief The MeasurementConfigurationModel class
 * Model that holds all available measurement configurations
 */
class OI_MAIN_EXPORT MeasurementConfigurationModel : public QAbstractListModel {
  Q_OBJECT

 public:
  explicit MeasurementConfigurationModel(QObject* parent = nullptr);

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

  // ##########################################
  // get informations about measurement configs
  // ##########################################

  [[nodiscard]] QModelIndex getIndex(const MeasurementConfig& mConfig) const;

  [[nodiscard]] MeasurementConfig getMeasurementConfig(
      const QModelIndex& index) const;

  [[nodiscard]] bool isUserConfig(const QModelIndex& index) const;

  // #################################
  // add or remove measurement configs
  // #################################

  QModelIndex addMeasurementConfig(const MeasurementConfig& mConfig);
  void removeMeasurementConfig(const QModelIndex& index);

  void replaceMeasurementConfig(const MeasurementConfigKey& oldKey,
                                const MeasurementConfig& mConfig);

  MeasurementConfig cloneMeasurementConfig(const MeasurementConfig& mConfig);

  // #######################################
  // get or set sensor configuration manager
  // #######################################

  [[nodiscard]] const QPointer<MeasurementConfigManager>&
  getMeasurementConfigurationManager() const;
  void setMeasurementConfigurationManager(
      const QPointer<MeasurementConfigManager>& manager);

 signals:

  // #########################
  // send messages to OpenIndy
  // #########################

  void sendMessage(const QString& msg, const MessageTypes& msgType,
                   const MessageDestinations& msgDest =
                       MessageDestinations::eConsoleMessage);

  // ########################################################
  // inform when the name of a measurement config has changed
  // ########################################################

  void measurementConfigNameChanged(const MeasurementConfig& mConfig);

 private slots:

  // ###########################################
  // update the model when configuration changes
  // ###########################################

  void updateModel();

 private:
  // ##############
  // helper methods
  // ##############

  void connectConfigManager();

  // #####################
  // sensor config manager
  // #####################

  QPointer<MeasurementConfigManager> measurementConfigManager;
};

#endif  // MEASUREMENTCONFIGURATIONMODEL_H

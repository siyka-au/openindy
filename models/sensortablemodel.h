#ifndef SENSORTABLEMODEL_H
#define SENSORTABLEMODEL_H

#include <QAbstractTableModel>
#include <QList>
#include <QObject>
#include <QPointer>

#include "oijob.h"
#include "oimetadata.h"
#include "pluginloader.h"
#include "pluginrepository.h"
#include "types.h"
#include "util.h"

using namespace oi;

/*!
 * \brief The SensorTableModel class
 * Model that holds all available sensor plugins
 */
class SensorTableModel : public QAbstractTableModel {
  Q_OBJECT

 public:
  explicit SensorTableModel(QObject* parent = nullptr);

  // ########################################
  // override methods of abstract table model
  // ########################################

  [[nodiscard]] int rowCount(
      const QModelIndex& parent = QModelIndex()) const override;
  [[nodiscard]] int columnCount(
      const QModelIndex& parent = QModelIndex()) const override;

  [[nodiscard]] QVariant data(const QModelIndex& index,
                              int role = Qt::DisplayRole) const override;

  [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation,
                                    int role = Qt::DisplayRole) const override;

  // ######################
  // get sensor information
  // ######################

  // get or set selection
  [[nodiscard]] QPair<QString, QString> getSelectedSensorPlugin() const;
  void selectSensorPlugin(const QModelIndex& index);
  void selectSensorPlugin(const SensorConfiguration& sConfig);
  void selectSensorPlugin(const QString& sensorName, const QString& pluginName);

  // get general sensor information
  [[nodiscard]] SensorType getSensorType() const;
  [[nodiscard]] SensorType getSensorType(const QModelIndex& index) const;
  [[nodiscard]] QString getSensorDescription() const;
  [[nodiscard]] QString getSensorDescription(const QModelIndex& index) const;
  [[nodiscard]] QString getPluginFilePath() const;
  [[nodiscard]] QString getPluginFilePath(const QModelIndex& index) const;

  // get supported connection / reading or action types
  [[nodiscard]] QList<ReadingTypes> getSupportedReadingTypes() const;
  [[nodiscard]] QList<SensorFunctions> getSupportedSensorActions() const;
  [[nodiscard]] QList<ConnectionKind> getSupportedConnectionTypes() const;

  // get default sensor config
  [[nodiscard]] SensorConfiguration getDefaultSensorConfig() const;

  // ######################################
  // get or set currently available plugins
  // ######################################

  [[nodiscard]] const QList<sys::Plugin>& getPlugins() const;
  void setPlugins(const QList<sys::Plugin>& plugins);

 signals:

  // #########################
  // send messages to OpenIndy
  // #########################

  void sendMessage(const QString& msg, const MessageTypes& msgType,
                   const MessageDestinations& msgDest =
                       MessageDestinations::eConsoleMessage);

  // ##################################
  // inform about sensor plugin changes
  // ##################################

  void selectedSensorPluginChanged();

 public slots:

  // ##############################################
  // update the model when the plugins were changed
  // ##############################################

  void updateModel();

 private:
  // ###########################
  // currently available plugins
  // ###########################

  QList<sys::Plugin> plugins;

  // ####################
  // save selected plugin
  // ####################

  QPair<QString, QString> selectedSensor;  // first=plugin, second=sensor
};

#endif  // SENSORTABLEMODEL_H

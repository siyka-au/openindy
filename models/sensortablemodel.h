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

  int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  int columnCount(const QModelIndex& parent = QModelIndex()) const override;

  QVariant data(const QModelIndex& index,
                int role = Qt::DisplayRole) const override;

  QVariant headerData(int section, Qt::Orientation orientation,
                      int role = Qt::DisplayRole) const override;

  // ######################
  // get sensor information
  // ######################

  // get or set selection
  QPair<QString, QString> getSelectedSensorPlugin() const;
  void selectSensorPlugin(const QModelIndex& index);
  void selectSensorPlugin(const SensorConfiguration& sConfig);
  void selectSensorPlugin(const QString& sensorName, const QString& pluginName);

  // get general sensor information
  SensorType getSensorType() const;
  SensorType getSensorType(const QModelIndex& index) const;
  QString getSensorDescription() const;
  QString getSensorDescription(const QModelIndex& index) const;
  QString getPluginFilePath() const;
  QString getPluginFilePath(const QModelIndex& index) const;

  // get supported connection / reading or action types
  QList<ReadingTypes> getSupportedReadingTypes() const;
  QList<SensorFunctions> getSupportedSensorActions() const;
  QList<ConnectionKind> getSupportedConnectionTypes() const;

  // get default sensor config
  SensorConfiguration getDefaultSensorConfig() const;

  // ######################################
  // get or set currently available plugins
  // ######################################

  const QList<sys::Plugin>& getPlugins() const;
  void setPlugins(const QList<sys::Plugin>& plugins);

 signals:

  // #########################
  // send messages to OpenIndy
  // #########################

  void sendMessage(const QString& msg, const MessageTypes& msgType,
                   const MessageDestinations& msgDest = eConsoleMessage);

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

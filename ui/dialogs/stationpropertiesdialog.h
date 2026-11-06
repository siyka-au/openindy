#ifndef STATIONPROPERTIESDIALOG_H
#define STATIONPROPERTIESDIALOG_H

#include <QAbstractButton>
#include <QAbstractItemView>
#include <QClipboard>
#include <QCloseEvent>
#include <QDialog>
#include <QPointer>
#include <QShowEvent>

#include "clipboardutil.h"
#include "modelmanager.h"
#include "sensoraccuracydelegate.h"
#include "sensorconfiguration.h"
#include "sensorparametersdelegate.h"

namespace Ui {
class StationPropertiesDialog;
}

/*!
 * \brief The StationPropertiesDialog class
 */
class StationPropertiesDialog : public QDialog {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(StationPropertiesDialog)

 public:
  explicit StationPropertiesDialog(QWidget* parent = nullptr);
  ~StationPropertiesDialog() override;

  // ##############################
  // get or set station information
  // ##############################

  // sensor config
  [[nodiscard]] const SensorConfiguration& getSensorConfiguration() const;
  void setSensorConfiguration(const SensorConfiguration& sConfig);

  // active station state
  [[nodiscard]] bool getIsActiveStation() const;
  void setIsActiveStation(bool isActiveStation);

 signals:

  // ##########################
  // signals to trigger actions
  // ##########################

  // select another sensor config for the active station
  void openSensorConfigurationDialog();

  // save changes made to the sensor config
  void sensorConfigurationChanged(const SensorConfiguration& sConfig);

  void sendMessage(const QString& msg, const MessageTypes& msgType,
                   const MessageDestinations& msgDest =
                       MessageDestinations::eConsoleMessage) const;

 private slots:

  // #########################
  // actions triggered by user
  // #########################

  // ok or cancel
  void on_buttonBox_clicked(QAbstractButton* button);

  // open sensor config dialog
  void on_pushButton_sensorConfig1_clicked();

  // key pressed
  void keyPressEvent(QKeyEvent* e) override;

  // sensor config changed
  void updateSensorAccuracy(const SensorConfiguration& sConfig);
  void updateSensorParameters(const SensorConfiguration& sConfig);

 private:
  Ui::StationPropertiesDialog* ui;

  // ##################################
  // methods to initialize GUI elements
  // ##################################

  void showEvent(QShowEvent* event) override;

  void initGUI();
  void initModels();

  // ##############
  // helper methods
  // ##############

  // copy values from and to clipboard
  void copyToClipboard();
  void pasteFromClipboard();

  // ######
  // models
  // ######

  QPointer<SensorAccuracyModel> accuracyModel;
  QPointer<SensorParametersModel> sensorParametersModel;

  // #################
  // helper attributes
  // #################

  SensorConfiguration sConfig;
  bool isActiveStation{};

  ClipBoardUtil clipBoardUtil;
};

#endif  // STATIONPROPERTIESDIALOG_H

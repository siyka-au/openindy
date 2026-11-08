#ifndef ACTUALPROPERTIESDIALOG_H
#define ACTUALPROPERTIESDIALOG_H

#include <QClipboard>
#include <QCloseEvent>
#include <QComboBox>
#include <QDialog>
#include <QFileDialog>
#include <QKeyEvent>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QMap>
#include <QMenu>
#include <QPointer>
#include <QShowEvent>
#include <QtNetwork/QNetworkInterface>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <vector>

#include "clipboard_util.h"
#include "function_statistic_model.h"
#include "model_manager.h"
#include "observation_table_delegate.h"
#include "reading_table_delegate.h"

using namespace oi;

namespace Ui {
class ActualPropertiesDialog;
}

/*!
 * \brief The ActualPropertiesDialog class
 */
class ActualPropertiesDialog : public QDialog {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(ActualPropertiesDialog)

 public:
  explicit ActualPropertiesDialog(QWidget* parent = nullptr);
  ~ActualPropertiesDialog() override;

 signals:

  // ###################
  // import observations
  // ###################

  void importObservations(const QString& filename);

  void useObservation(bool use, const QModelIndex& index);
  void unUseObservation(bool use, const QModelIndex& index);
  void removeObservationsById(const std::vector<int> selectedIds);

  void sendMessage(const QString& msg, const MessageTypes& msgType,
                   const MessageDestinations& msgDest =
                       MessageDestinations::eConsoleMessage) const;

 private slots:

  // #########################
  // actions triggered by user
  // #########################

  // key pressed
  void keyPressEvent(QKeyEvent* e) override;

  // import observations
  void on_tableView_observation_customContextMenuRequested(const QPoint& pos);
  void importObservationsMenuClicked(bool checked);

  // resize table views
  void resizeTableView();

  // function statistic
  void on_comboBox_displayedFunction_currentIndexChanged(int index);

  // ##############
  // helper methods
  // ##############

  // copy values from and to clipboard
  void copyToClipboard();

  void on_tabWidget_selectedFeature_customContextMenuRequested(
      const QPoint& pos);

  void useObservations();
  void unUseObservations();
  void deleteObservations();

 private:
  // ##################################
  // methods to initialize GUI elements
  // ##################################

  void showEvent(QShowEvent* event) override;

  void initGUI();
  void initModels();
  QModelIndexList getSelection();
  void useObservations(bool use);

  // #################
  // helper attributes
  // #################

  QPointer<FunctionStatisticModel> functionStatisticModel;

  ClipBoardUtil clipBoardUtil;

 private:
  Ui::ActualPropertiesDialog* ui;
};

#endif  // ACTUALPROPERTIESDIALOG_H

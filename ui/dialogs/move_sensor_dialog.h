#ifndef OI_APP_UI_DIALOGS_MOVE_SENSOR_DIALOG_H_
#define OI_APP_UI_DIALOGS_MOVE_SENSOR_DIALOG_H_

#include <QDialog>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QShowEvent>

#include "model_manager.h"
#include "reading.h"
#include "types.h"

using namespace oi;

namespace Ui {
class MoveSensorDialog;
}

/*!
 * \brief The MovementDialog class
 * Dialog to specify a reading the active station's sensor shall be moved to
 */
class MoveSensorDialog : public QDialog {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(MoveSensorDialog)

 public:
  explicit MoveSensorDialog(QWidget* parent = nullptr);
  ~MoveSensorDialog() override;

 signals:

  // ################################
  // move the active station's sensor
  // ################################

  void moveSensor(const Reading& reading);

 private slots:

  // #########################
  // actions triggered by user
  // #########################

  // move sensor
  void on_pushButton_move_clicked();

  // reading type changed
  void on_comboBox_typeOfReading_currentTextChanged(const QString& arg1);

 private:
  // ##################################
  // methods to initialize GUI elements
  // ##################################

  void initGUI();
  void initModels();
  void showEvent(QShowEvent* event) override;

 private:
  // #################
  // helper attributes
  // #################

  // regular expressions to validate user input
  QRegularExpression validDouble;
  QRegularExpressionValidator* doubleValidator;

  Ui::MoveSensorDialog* ui;
};

#endif  // OI_APP_UI_DIALOGS_MOVE_SENSOR_DIALOG_H_

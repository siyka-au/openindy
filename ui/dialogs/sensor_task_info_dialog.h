#ifndef SENSORTASKINFODIALOG_H
#define SENSORTASKINFODIALOG_H

#include <QDialog>
#include <QShowEvent>

namespace Ui {
class SensorTaskInfoDialog;
}

/*!
 * \brief The SensorTaskInfoDialog class
 * Dialog that appears whenever a sensor task is running
 */
class SensorTaskInfoDialog : public QDialog {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(SensorTaskInfoDialog)

 public:
  explicit SensorTaskInfoDialog(QWidget* parent = nullptr);

  ~SensorTaskInfoDialog() override;

  // ################
  // set display text
  // ################

  void setDisplayMessage(const QString& msg);
  void enableFinishButton(
      const bool enabled);  // currently used for finish scan

 signals:
  void finishMeasurement();

 private slots:

  // #########################
  // actions triggered by user
  // #########################

  void on_pushButton_cancel_clicked();

  void on_pushButton_finish_clicked();

 private:
  Ui::SensorTaskInfoDialog* ui;
};

#endif  // SENSORTASKINFODIALOG_H

#ifndef OI_APP_UI_DIALOGS_LOADING_DIALOG_H_
#define OI_APP_UI_DIALOGS_LOADING_DIALOG_H_

#include <QCloseEvent>
#include <QDialog>
#include <QShowEvent>

namespace Ui {
class LoadingDialog;
}

/*!
 * \brief The LoadingDialog class
 * Display a loading screen when performing time consuming tasks
 */
class LoadingDialog : public QDialog {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(LoadingDialog)

 public:
  explicit LoadingDialog(QWidget* parent = nullptr);
  ~LoadingDialog() override;

 public slots:

  // ###################################
  // update progress and display message
  // ###################################

  void setProgress(int percent,
                   const QString& msg) const;  // progress (0-100)

 private:
  // ##################################
  // methods to initialize GUI elements
  // ##################################

  void closeEvent(QCloseEvent* event) override;

 private:
  Ui::LoadingDialog* ui;
};

#endif  // OI_APP_UI_DIALOGS_LOADING_DIALOG_H_

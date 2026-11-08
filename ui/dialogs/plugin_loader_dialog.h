#ifndef OI_APP_UI_DIALOGS_PLUGIN_LOADER_DIALOG_H_
#define OI_APP_UI_DIALOGS_PLUGIN_LOADER_DIALOG_H_

#include <QDialog>
#include <QFile>
#include <QFileDialog>
#include <QShowEvent>
#include <QtWidgets/QMessageBox>

#include "plugin_manager.h"

using namespace oi;

namespace Ui {
class PluginLoaderDialog;
}

/*!
 * \brief The PluginLoaderDialog class
 */
class PluginLoaderDialog : public QDialog {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(PluginLoaderDialog)

 public:
  explicit PluginLoaderDialog(QWidget* parent = nullptr);
  ~PluginLoaderDialog() override;

 signals:

  // #############
  // send messages
  // #############

  void sendMessage(const QString& msg, const MessageTypes& msgType,
                   const MessageDestinations& msgDest =
                       MessageDestinations::eConsoleMessage);

 private slots:

  // #########################
  // actions triggered by user
  // #########################

  void on_toolButton_Path_clicked();
  void on_pushButton_Cancel_clicked();
  void on_pushButton_Ok_clicked();

  // #######################
  // task finished callbacks
  // #######################

  void importFinished(const bool& success);

 private:
  Ui::PluginLoaderDialog* ui;
};

#endif  // OI_APP_UI_DIALOGS_PLUGIN_LOADER_DIALOG_H_

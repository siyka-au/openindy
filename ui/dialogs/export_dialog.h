#ifndef OI_APP_UI_DIALOGS_EXPORT_DIALOG_H_
#define OI_APP_UI_DIALOGS_EXPORT_DIALOG_H_

#include <QDialog>
#include <QFileDialog>
#include <QMessageBox>
#include <QPointer>
#include <QShowEvent>
#include <QStringListModel>

#include "exchange_params.h"
#include "model_manager.h"
#include "types.h"
#include "util.h"

using namespace oi;

namespace Ui {
class ExportDialog;
}

/*!
 * \brief The ExportDialog class
 */
class ExportDialog : public QDialog {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(ExportDialog)

 public:
  explicit ExportDialog(QWidget* parent = nullptr);
  ~ExportDialog() override;

 signals:

  void startExport(const ExchangeParams& params);

 private slots:
  void on_pushButton_export_clicked();

  void on_pushButton_Path_clicked();

  void on_comboBox_plugin_currentTextChanged(const QString& arg1);

 private:
  // ##################################
  // methods to initialize GUI elements
  // ##################################

  void showEvent(QShowEvent* event) override;

  void initModels();

  Ui::ExportDialog* ui;
};

#endif  // OI_APP_UI_DIALOGS_EXPORT_DIALOG_H_

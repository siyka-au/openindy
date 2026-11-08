#ifndef OI_APP_UI_DIALOGS_IMPORT_NOMINAL_DIALOG_H_
#define OI_APP_UI_DIALOGS_IMPORT_NOMINAL_DIALOG_H_

#include <QDialog>
#include <QFileDialog>
#include <QMessageBox>
#include <QPointer>
#include <QShowEvent>
#include <QStringListModel>

#include "exchange_params.h"
#include "model_manager.h"
#include "project_config.h"
#include "types.h"
#include "util.h"

using namespace oi;

namespace Ui {
class ImportNominalDialog;
}

/*!
 * \brief The ImportNominalDialog class
 */
class ImportNominalDialog : public QDialog {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(ImportNominalDialog)

 public:
  explicit ImportNominalDialog(QWidget* parent = nullptr);
  ~ImportNominalDialog() override;

 signals:

  // ############
  // start import
  // ############

  void startImport(const ExchangeParams& params);

 private slots:

  // #########################
  // actions triggered by user
  // #########################

  void on_comboBox_plugin_sa_currentTextChanged(const QString& arg1);
  void on_comboBox_exchange_sa_currentTextChanged(const QString& arg1);
  void on_pushButton_file_sa_clicked();
  void on_pushButton_cancel_sa_clicked();
  void on_pushButton_import_sa_clicked();

 private:
  // ##################################
  // methods to initialize GUI elements
  // ##################################

  void showEvent(QShowEvent* event) override;

  void initModels();

 private:
  Ui::ImportNominalDialog* ui;
};

#endif  // OI_APP_UI_DIALOGS_IMPORT_NOMINAL_DIALOG_H_

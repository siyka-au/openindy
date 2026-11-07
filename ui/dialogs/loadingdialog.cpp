#include "loadingdialog.h"

#include "ui_loadingdialog.h"

/*!
 * \brief LoadingDialog::LoadingDialog
 * \param parent
 */
LoadingDialog::LoadingDialog(QWidget* parent)
    : QDialog(parent, Qt::CustomizeWindowHint | Qt::WindowTitleHint |
                          Qt::WindowMinMaxButtonsHint),
      ui(new Ui::LoadingDialog) {
  ui->setupUi(this);
  this->setModal(true);
}

/*!
 * \brief LoadingDialog::~LoadingDialog
 */
LoadingDialog::~LoadingDialog() { delete ui; }

/*!
 * \brief LoadingDialog::setProgress
 * Shows a report of a QtProgressSink
 * \param percent 0..100, or -1 when the operation cannot say how far it is
 * \param msg
 */
void LoadingDialog::setProgress(int percent, const QString& msg) const {
  this->ui->label_description->setText(msg);
  if (percent < 0) {
    this->ui->progressBar->setRange(0, 0);  // busy indicator
  } else {
    this->ui->progressBar->setRange(0, 100);
    this->ui->progressBar->setValue(percent);
  }
}

/*!
 * \brief LoadingDialog::closeEvent
 * \param event
 */
void LoadingDialog::closeEvent(QCloseEvent* event) {
  // reset display text and progress bar
  this->ui->label_description->setText("");
  this->ui->progressBar->setValue(0);

  event->accept();
}

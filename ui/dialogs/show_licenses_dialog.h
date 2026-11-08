#ifndef SHOWLICENSESDIALOG_H
#define SHOWLICENSESDIALOG_H

#include <QDebug>
#include <QDialog>
#include <QDir>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QWidget>

namespace Ui {
class ShowLicensesDialog;
}

class ShowLicensesDialog : public QDialog {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(ShowLicensesDialog)

 public:
  explicit ShowLicensesDialog(QWidget* parent = nullptr);
  ~ShowLicensesDialog() override;

 public slots:
  void onClick(const QModelIndex&);

 private:
  Ui::ShowLicensesDialog* ui;

  QStandardItemModel model;
};

#endif  // SHOWLICENSESDIALOG_H

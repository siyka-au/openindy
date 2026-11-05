#ifndef ABOUTDIALOG_H
#define ABOUTDIALOG_H

#ifndef OPENINDY_BUILD
#define OPENINDY_BUILD -1
#endif

#include <QDialog>
#include <QShowEvent>

namespace Ui {
class AboutDialog;
}

/*!
 * \brief The AboutDialog class
 * Dialog that shows version information about OpenIndy
 */
class AboutDialog : public QDialog {
  Q_OBJECT

 public:
  explicit AboutDialog(QWidget* parent = nullptr);
  ~AboutDialog() override;

 private:
  Ui::AboutDialog* ui;
};

#endif  // ABOUTDIALOG_H

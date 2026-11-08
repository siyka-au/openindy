#ifndef OI_APP_UI_DIALOGS_ABOUT_DIALOG_H_
#define OI_APP_UI_DIALOGS_ABOUT_DIALOG_H_

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
  Q_DISABLE_COPY_MOVE(AboutDialog)

 public:
  explicit AboutDialog(QWidget* parent = nullptr);
  ~AboutDialog() override;

 private:
  Ui::AboutDialog* ui;
};

#endif  // OI_APP_UI_DIALOGS_ABOUT_DIALOG_H_

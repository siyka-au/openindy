#include "plugin_loader_dialog.h"

#include "qt_convert.h"
#include "ui_plugin_loader_dialog.h"

/*!
 * \brief PluginLoaderDialog::PluginLoaderDialog
 * \param parent
 */
PluginLoaderDialog::PluginLoaderDialog(QWidget* parent)
    : QDialog(parent), ui(new Ui::PluginLoaderDialog) {
  ui->setupUi(this);

  // connect plugin loader
  QObject::connect(PluginManager::getInstance().data(),
                   &PluginManager::updateProgress, this->ui->progressBar,
                   &QProgressBar::setValue, Qt::AutoConnection);
  QObject::connect(PluginManager::getInstance().data(),
                   &PluginManager::importFinished, this,
                   &PluginLoaderDialog::importFinished, Qt::AutoConnection);
}

/*!
 * \brief PluginLoaderDialog::~PluginLoaderDialog
 */
PluginLoaderDialog::~PluginLoaderDialog() { delete ui; }

/*!
 * \brief PluginLoaderDialog::on_toolButton_Path_clicked
 */
void PluginLoaderDialog::on_toolButton_Path_clicked() {
  // let the user choose a folder
  QString pluginDir = QFileDialog::getExistingDirectory(
      this, tr("Open Directory"), "/home",
      QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

  // get and check plugin meta data as a preview
  PluginMetaData metaData = PluginManager::getPluginMetaData(pluginDir);
  if (metaData.iid.compare("") == 0) {
    Console::getInstance()->addLine(
        QString("No plugin found at path: %1").arg(pluginDir),
        MessageTypes::eErrorMessage);
    return;
  }

  // show meta info in GUI
  this->ui->lineEdit_Path->setText(toQString(metaData.path));
  this->ui->lineEdit_Name->setText(toQString(metaData.name));
  this->ui->lineEdit_pluginType->setText(toQString(metaData.iid));

  this->ui->textBrowser_description->clear();
  this->ui->textBrowser_description->append("<b>name</b>: " +
                                            toQString(metaData.name));
  this->ui->textBrowser_description->append("<b>author</b>: " +
                                            toQString(metaData.author));
  this->ui->textBrowser_description->append("<b>version</b>: " +
                                            toQString(metaData.pluginVersion));
  this->ui->textBrowser_description->append(
      "<b>operating system</b>: " + toQString(metaData.operatingSystem));
  this->ui->textBrowser_description->append(
      toQString("<b>compiler</b>: " + metaData.compiler + "<hr /><p />"));

  this->ui->progressBar->setMinimum(0);
  this->ui->progressBar->setMaximum(100);
  this->ui->progressBar->setValue(0);

  if (metaData.dependencies) {
    this->ui->textBrowser_description->append(
        "<b>dependencies</b>:" +
        QString::number(metaData.dependenciesPath.size()));
  } else {
    this->ui->textBrowser_description->append("<b>dependencies</b>: none");
  }

  for (const oi::PluginDependency& dependency : metaData.dependenciesPath) {
    QString depName = toQString(dependency.name);
    QString depPath = toQString(dependency.path);

    QString tmpText(QString("   -" + depName + ": " + depPath));

    this->ui->textBrowser_description->append(tmpText);
  }

  this->ui->pushButton_Ok->setEnabled(true);
}

/*!
 * \brief PluginLoaderDialog::on_pushButton_Cancel_clicked
 */
void PluginLoaderDialog::on_pushButton_Cancel_clicked() { this->close(); }

/*!
 * \brief PluginLoaderDialog::on_pushButton_Ok_clicked
 */
void PluginLoaderDialog::on_pushButton_Ok_clicked() {
  if (this->ui->lineEdit_Path->text().compare("") != 0) {
    PluginManager::importPlugin(this->ui->lineEdit_Path->text());
    this->ui->textBrowser_description->append("<hr /><p />");
  }
}

/*!
 * \brief PluginLoaderDialog::importFinished
 * \param success
 */
void PluginLoaderDialog::importFinished(const bool& success) {
  if (success) {
    this->ui->progressBar->setValue(this->ui->progressBar->maximum());
    QMessageBox::information(this, "success",
                             "plugin has been copied successfully.");
  } else {
    this->ui->progressBar->setValue(ui->progressBar->minimum());
    QMessageBox::critical(this, "error",
                          "plugin has not been copied successfully.");
  }
}

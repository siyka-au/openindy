#include "trafo_param_properties_dialog.h"

#include <map>
#include <vector>

#include "containers.h"
#include "ui_trafo_param_properties_dialog.h"

/*!
 * \brief TrafoParamPropertiesDialog::TrafoParamPropertiesDialog
 * \param parent
 */
TrafoParamPropertiesDialog::TrafoParamPropertiesDialog(QWidget* parent)
    : QDialog(parent), ui(new Ui::TrafoParamPropertiesDialog) {
  ui->setupUi(this);
}

/*!
 * \brief TrafoParamPropertiesDialog::~TrafoParamPropertiesDialog
 */
TrafoParamPropertiesDialog::~TrafoParamPropertiesDialog() { delete ui; }

/*!
 * \brief TrafoParamPropertiesDialog::getId
 * \return
 */
const int& TrafoParamPropertiesDialog::getId() const { return this->id; }

/*!
 * \brief TrafoParamPropertiesDialog::setId
 * \param id
 */
void TrafoParamPropertiesDialog::setId(const int& id) { this->id = id; }

/*!
 * \brief TrafoParamPropertiesDialog::getName
 * \return
 */
const QString& TrafoParamPropertiesDialog::getName() const {
  return this->name;
}

/*!
 * \brief TrafoParamPropertiesDialog::setName
 * \param name
 */
void TrafoParamPropertiesDialog::setName(const QString& name) {
  this->name = name;
}

/*!
 * \brief TrafoParamPropertiesDialog::setCurrentTrafoParam
 * \param id
 * \param name
 */
void TrafoParamPropertiesDialog::setCurrentTrafoParam(const int& id,
                                                      const QString& name) {
  this->id = id;
  this->name = name;
}

/*!
 * \brief TrafoParamPropertiesDialog::getUnknownTrafoParamParameters
 * \return
 */
const std::map<TrafoParamParameters, QString>&
TrafoParamPropertiesDialog::getUnknownTrafoParamParameters() const {
  return this->parameters;
}

/*!
 * \brief TrafoParamPropertiesDialog::setUnknownTrafoParamParameters
 * \param parameters
 */
void TrafoParamPropertiesDialog::setUnknownTrafoParamParameters(
    const std::map<TrafoParamParameters, QString>& parameters) {
  this->parameters = parameters;
}

/*!
 * \brief TrafoParamPropertiesDialog::on_toolButton_cancel_clicked
 */
void TrafoParamPropertiesDialog::on_toolButton_cancel_clicked() {
  this->close();
}

/*!
 * \brief TrafoParamPropertiesDialog::on_toolButton_ok_clicked
 */
void TrafoParamPropertiesDialog::on_toolButton_ok_clicked() {
  QMessageBox msgBox;
  msgBox.setText(
      "Editing transformation parameters may result in invalid data!!");
  msgBox.setInformativeText("Do you want to continue?");
  msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
  msgBox.setDefaultButton(QMessageBox::No);
  int ret = msgBox.exec();

  if (ret == QMessageBox::Yes) {
    // get a map of unknown parameters
    std::map<TrafoParamParameters, double> parameters;
    this->getParametersFromGUI(parameters);

    emit this->trafoParamParametersChanged(this->id, parameters);
  }

  this->close();
}

/*!
 * \brief TrafoParamPropertiesDialog::showEvent
 * \param event
 */
void TrafoParamPropertiesDialog::showEvent(QShowEvent* event) {
  // update GUI with respect to the current feature type
  this->initGUI();

  event->accept();
}

/*!
 * \brief TrafoParamPropertiesDialog::initGUI
 */
void TrafoParamPropertiesDialog::initGUI() {
  // get a list of all available unknown parameters
  std::vector<TrafoParamParameters> availableUnknownParameters =
      getAvailableTrafoParamParameters();

  // setdefault inputs
  foreach (const TrafoParamParameters& param, availableUnknownParameters) {
    this->setParametersToGUI(param, oi::value(this->parameters, param, "0.0"));
  }
}

/*!
 * \brief TrafoParamPropertiesDialog::initModels
 */
void TrafoParamPropertiesDialog::initModels() {}

/*!
 * \brief TrafoParamPropertiesDialog::setParametersToGUI
 * \param param
 * \param value
 */
void TrafoParamPropertiesDialog::setParametersToGUI(
    const TrafoParamParameters& param, const QString& value) {
  switch (param) {
    case TrafoParamParameters::eUnknownTX:
      this->ui->lineEdit_tx->setText(value);
      break;
    case TrafoParamParameters::eUnknownTY:
      this->ui->lineEdit_ty->setText(value);
      break;
    case TrafoParamParameters::eUnknownTZ:
      this->ui->lineEdit_tz->setText(value);
      break;
    case TrafoParamParameters::eUnknownRX:
      this->ui->lineEdit_rx->setText(value);
      break;
    case TrafoParamParameters::eUnknownRY:
      this->ui->lineEdit_ry->setText(value);
      break;
    case TrafoParamParameters::eUnknownRZ:
      this->ui->lineEdit_rz->setText(value);
      break;
    case TrafoParamParameters::eUnknownSX:
      this->ui->lineEdit_sx->setText(value);
      break;
    case TrafoParamParameters::eUnknownSY:
      this->ui->lineEdit_sy->setText(value);
      break;
    case TrafoParamParameters::eUnknownSZ:
      this->ui->lineEdit_sz->setText(value);
      break;
  }
}

/*!
 * \brief TrafoParamPropertiesDialog::getParametersFromGUI
 * \param parameters
 */
void TrafoParamPropertiesDialog::getParametersFromGUI(
    std::map<TrafoParamParameters, double>& parameters) {
  // get a list of all unknown parameters
  std::vector<TrafoParamParameters> unknownParameters =
      getAvailableTrafoParamParameters();

  // get parameter display config
  const ParameterDisplayConfig& dConfig =
      ModelManager::getParameterDisplayConfig();

  foreach (const TrafoParamParameters& param, unknownParameters) {
    // get values from GUI elements (only use values that differ from the
    // initial input)
    switch (param) {
      case TrafoParamParameters::eUnknownTX:
        if (this->parameters.contains(param) &&
            oi::value(this->parameters, param)
                    .compare(this->ui->lineEdit_tx->text()) == 0) {
          break;
        }
        parameters.insert_or_assign(
            param,
            convertToDefault(this->ui->lineEdit_tx->text().toDouble(),
                             dConfig.getDisplayUnit(DimensionType::eMetric)));
        break;
      case TrafoParamParameters::eUnknownTY:
        if (this->parameters.contains(param) &&
            oi::value(this->parameters, param)
                    .compare(this->ui->lineEdit_ty->text()) == 0) {
          break;
        }
        parameters.insert_or_assign(
            param,
            convertToDefault(this->ui->lineEdit_ty->text().toDouble(),
                             dConfig.getDisplayUnit(DimensionType::eMetric)));
        break;
      case TrafoParamParameters::eUnknownTZ:
        if (this->parameters.contains(param) &&
            oi::value(this->parameters, param)
                    .compare(this->ui->lineEdit_tz->text()) == 0) {
          break;
        }
        parameters.insert_or_assign(
            param,
            convertToDefault(this->ui->lineEdit_tz->text().toDouble(),
                             dConfig.getDisplayUnit(DimensionType::eMetric)));
        break;
      case TrafoParamParameters::eUnknownRX:
        if (this->parameters.contains(param) &&
            oi::value(this->parameters, param)
                    .compare(this->ui->lineEdit_rx->text()) == 0) {
          break;
        }
        parameters.insert_or_assign(
            param,
            convertToDefault(this->ui->lineEdit_rx->text().toDouble(),
                             dConfig.getDisplayUnit(DimensionType::eAngular)));
        break;
      case TrafoParamParameters::eUnknownRY:
        if (this->parameters.contains(param) &&
            oi::value(this->parameters, param)
                    .compare(this->ui->lineEdit_ry->text()) == 0) {
          break;
        }
        parameters.insert_or_assign(
            param,
            convertToDefault(this->ui->lineEdit_ry->text().toDouble(),
                             dConfig.getDisplayUnit(DimensionType::eAngular)));
        break;
      case TrafoParamParameters::eUnknownRZ:
        if (this->parameters.contains(param) &&
            oi::value(this->parameters, param)
                    .compare(this->ui->lineEdit_rz->text()) == 0) {
          break;
        }
        parameters.insert_or_assign(
            param,
            convertToDefault(this->ui->lineEdit_rz->text().toDouble(),
                             dConfig.getDisplayUnit(DimensionType::eAngular)));
        break;
      case TrafoParamParameters::eUnknownSX:
        if (this->parameters.contains(param) &&
            oi::value(this->parameters, param)
                    .compare(this->ui->lineEdit_sx->text()) == 0) {
          break;
        }
        parameters.insert_or_assign(param,
                                    this->ui->lineEdit_sx->text().toDouble());
        break;
      case TrafoParamParameters::eUnknownSY:
        if (this->parameters.contains(param) &&
            oi::value(this->parameters, param)
                    .compare(this->ui->lineEdit_sy->text()) == 0) {
          break;
        }
        parameters.insert_or_assign(param,
                                    this->ui->lineEdit_sy->text().toDouble());
        break;
      case TrafoParamParameters::eUnknownSZ:
        if (this->parameters.contains(param) &&
            oi::value(this->parameters, param)
                    .compare(this->ui->lineEdit_sz->text()) == 0) {
          break;
        }
        parameters.insert_or_assign(param,
                                    this->ui->lineEdit_sz->text().toDouble());
        break;
    }
  }
}

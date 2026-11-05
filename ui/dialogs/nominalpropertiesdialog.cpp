#include "nominalpropertiesdialog.h"

#include "ui_nominalpropertiesdialog.h"

/*!
 * \brief NominalPropertiesDialog::NominalPropertiesDialog
 * \param parent
 */
NominalPropertiesDialog::NominalPropertiesDialog(QWidget* parent)
    : QDialog(parent), ui(new Ui::NominalPropertiesDialog) {
  this->ui->setupUi(this);

  // init GUI elements and assign models
  this->initGUI();
  this->initModels();
}

/*!
 * \brief NominalPropertiesDialog::~NominalPropertiesDialog
 */
NominalPropertiesDialog::~NominalPropertiesDialog() { delete this->ui; }

/*!
 * \brief NominalPropertiesDialog::getId
 * \return
 */
const int& NominalPropertiesDialog::getId() const { return this->id; }

/*!
 * \brief NominalPropertiesDialog::setId
 * \param id
 */
void NominalPropertiesDialog::setId(const int& id) { this->id = id; }

/*!
 * \brief NominalPropertiesDialog::getName
 * \return
 */
const QString& NominalPropertiesDialog::getName() const { return this->name; }

/*!
 * \brief NominalPropertiesDialog::setName
 * \param name
 */
void NominalPropertiesDialog::setName(const QString& name) {
  this->name = name;
}

/*!
 * \brief NominalPropertiesDialog::getType
 * \return
 */
const FeatureTypes& NominalPropertiesDialog::getType() const {
  return this->type;
}

/*!
 * \brief NominalPropertiesDialog::setType
 * \param type
 */
void NominalPropertiesDialog::setType(const FeatureTypes& type) {
  this->type = type;
}

/*!
 * \brief NominalPropertiesDialog::setCurrentNominal
 * \param id
 * \param name
 * \param type
 */
void NominalPropertiesDialog::setCurrentNominal(const int& id,
                                                const QString& name,
                                                const FeatureTypes& type) {
  this->id = id;
  this->name = name;
  this->type = type;
}

/*!
 * \brief NominalPropertiesDialog::getUnknownNominalParameters
 * \return
 */
const QMap<GeometryParameters, QString>&
NominalPropertiesDialog::getUnknownNominalParameters() const {
  return this->parameters;
}

/*!
 * \brief NominalPropertiesDialog::setUnknownNominalParameters
 * \param parameters
 */
void NominalPropertiesDialog::setUnknownNominalParameters(
    const QMap<GeometryParameters, QString>& parameters) {
  this->parameters = parameters;
}

/*!
 * \brief NominalPropertiesDialog::on_toolButton_cancel_clicked
 */
void NominalPropertiesDialog::on_toolButton_cancel_clicked() { this->close(); }

/*!
 * \brief NominalPropertiesDialog::on_toolButton_ok_clicked
 */
void NominalPropertiesDialog::on_toolButton_ok_clicked() {
  // get a map of unknown parameters
  QMap<GeometryParameters, double> parameters;
  this->getParametersFromGUI(parameters);

  emit this->nominalParametersChanged(this->id, parameters);

  this->close();
}

/*!
 * \brief NominalPropertiesDialog::showEvent
 * \param event
 */
void NominalPropertiesDialog::showEvent(QShowEvent* event) {
  // update GUI with respect to the current feature type
  this->initGUI();

  event->accept();
}

/*!
 * \brief NominalPropertiesDialog::initGUI
 */
void NominalPropertiesDialog::initGUI() {
  // check if the current nominal is a geometry
  if (!getIsGeometry(this->type)) {
    return;
  }
  GeometryTypes type = getGeometryTypeEnum(this->type);

  // get a list of all available unknown parameters
  QList<GeometryParameters> availableUnknownParameters =
      getAvailableGeometryParameters();

  // get a list of unknown parameters that correspond to the current geometry
  // type
  QList<GeometryParameters> currentUnknownParameters =
      getGeometryParameters(type);

  // filter GUI elements depending on the type of geometry
  foreach (const GeometryParameters& param, availableUnknownParameters) {
    // set visibility
    if (currentUnknownParameters.contains(param)) {  // set visible
      this->setVisibility(param, true);
    } else {  // set invisible
      this->setVisibility(param, false);
    }

    // set default input
    this->setParametersToGUI(param, this->parameters.value(param, "0.0"));
  }
}

/*!
 * \brief NominalPropertiesDialog::initModels
 */
void NominalPropertiesDialog::initModels() {}

void NominalPropertiesDialog::setVisibility(const GeometryParameters& param,
                                            const bool& visible) {
  switch (param) {
    case GeometryParameters::eUnknownX:
      this->ui->label_nominalX->setVisible(visible);
      this->ui->lineEdit_nominalX->setVisible(visible);
      break;
    case GeometryParameters::eUnknownY:
      this->ui->label_nominalY->setVisible(visible);
      this->ui->lineEdit_nominalY->setVisible(visible);
      break;
    case GeometryParameters::eUnknownZ:
      this->ui->label_nominalZ->setVisible(visible);
      this->ui->lineEdit_nominalZ->setVisible(visible);
      break;
    case GeometryParameters::eUnknownPrimaryI:
      this->ui->label_nominalI1->setVisible(visible);
      this->ui->lineEdit_nominalI1->setVisible(visible);
      break;
    case GeometryParameters::eUnknownPrimaryJ:
      this->ui->label_nominalJ1->setVisible(visible);
      this->ui->lineEdit_nominalJ1->setVisible(visible);
      break;
    case GeometryParameters::eUnknownPrimaryK:
      this->ui->label_nominalK1->setVisible(visible);
      this->ui->lineEdit_nominalK1->setVisible(visible);
      break;
    case GeometryParameters::eUnknownSecondaryI:
      this->ui->label_nominalI2->setVisible(visible);
      this->ui->lineEdit_nominalI2->setVisible(visible);
      break;
    case GeometryParameters::eUnknownSecondaryJ:
      this->ui->label_nominalJ2->setVisible(visible);
      this->ui->lineEdit_nominalJ2->setVisible(visible);
      break;
    case GeometryParameters::eUnknownSecondaryK:
      this->ui->label_nominalK2->setVisible(visible);
      this->ui->lineEdit_nominalK2->setVisible(visible);
      break;
    case GeometryParameters::eUnknownRadiusA:
      this->ui->label_nominalR1->setVisible(visible);
      this->ui->lineEdit_nominalR1->setVisible(visible);
      break;
    case GeometryParameters::eUnknownRadiusB:
      this->ui->label_nominalR2->setVisible(visible);
      this->ui->lineEdit_nominalR2->setVisible(visible);
      break;
    case GeometryParameters::eUnknownAperture:
      this->ui->label_nominalAperture->setVisible(visible);
      this->ui->lineEdit_nominalAperture->setVisible(visible);
      break;
    case GeometryParameters::eUnknownA:
      this->ui->label_nominalA->setVisible(visible);
      this->ui->lineEdit_nominalA->setVisible(visible);
      break;
    case GeometryParameters::eUnknownB:
      this->ui->label_nominalB->setVisible(visible);
      this->ui->lineEdit_nominalB->setVisible(visible);
      break;
    case GeometryParameters::eUnknownC:
      this->ui->label_nominalC->setVisible(visible);
      this->ui->lineEdit_nominalC->setVisible(visible);
      break;
    case GeometryParameters::eUnknownAngle:
      this->ui->label_nominalAngle->setVisible(visible);
      this->ui->lineEdit_nominalAngle->setVisible(visible);
      break;
    case GeometryParameters::eUnknownDistance:
      this->ui->label_nominalDistance->setVisible(visible);
      this->ui->lineEdit_nominalDistance->setVisible(visible);
      break;
    case GeometryParameters::eUnknownMeasurementSeries:
      this->ui->label_nominalMeasurementSeries->setVisible(visible);
      this->ui->lineEdit_nominalMeasurementSeries->setVisible(visible);
      break;
    case GeometryParameters::eUnknownTemperature:
      this->ui->label_nominalTemperature->setVisible(visible);
      this->ui->lineEdit_nominalTemperature->setVisible(visible);
      break;
    case GeometryParameters::eUnknownLength:
      this->ui->label_nominalLength->setVisible(visible);
      this->ui->lineEdit_nominalLength->setVisible(visible);
      break;
  }
}

/*!
 * \brief NominalPropertiesDialog::setParametersToGUI
 * \param param
 * \param value
 */
void NominalPropertiesDialog::setParametersToGUI(
    const GeometryParameters& param, const QString& value) {
  switch (param) {
    case GeometryParameters::eUnknownX:
      this->ui->lineEdit_nominalX->setText(value);
      break;
    case GeometryParameters::eUnknownY:
      this->ui->lineEdit_nominalY->setText(value);
      break;
    case GeometryParameters::eUnknownZ:
      this->ui->lineEdit_nominalZ->setText(value);
      break;
    case GeometryParameters::eUnknownPrimaryI:
      this->ui->lineEdit_nominalI1->setText(value);
      break;
    case GeometryParameters::eUnknownPrimaryJ:
      this->ui->lineEdit_nominalJ1->setText(value);
      break;
    case GeometryParameters::eUnknownPrimaryK:
      this->ui->lineEdit_nominalK1->setText(value);
      break;
    case GeometryParameters::eUnknownSecondaryI:
      this->ui->lineEdit_nominalI2->setText(value);
      break;
    case GeometryParameters::eUnknownSecondaryJ:
      this->ui->lineEdit_nominalJ2->setText(value);
      break;
    case GeometryParameters::eUnknownSecondaryK:
      this->ui->lineEdit_nominalK2->setText(value);
      break;
    case GeometryParameters::eUnknownRadiusA:
      this->ui->lineEdit_nominalR1->setText(value);
      break;
    case GeometryParameters::eUnknownRadiusB:
      this->ui->lineEdit_nominalR2->setText(value);
      break;
    case GeometryParameters::eUnknownAperture:
      this->ui->lineEdit_nominalAperture->setText(value);
      break;
    case GeometryParameters::eUnknownA:
      this->ui->lineEdit_nominalA->setText(value);
      break;
    case GeometryParameters::eUnknownB:
      this->ui->lineEdit_nominalB->setText(value);
      break;
    case GeometryParameters::eUnknownC:
      this->ui->lineEdit_nominalC->setText(value);
      break;
    case GeometryParameters::eUnknownAngle:
      this->ui->lineEdit_nominalAngle->setText(value);
      break;
    case GeometryParameters::eUnknownDistance:
      this->ui->lineEdit_nominalDistance->setText(value);
      break;
    case GeometryParameters::eUnknownMeasurementSeries:
      this->ui->lineEdit_nominalMeasurementSeries->setText(value);
      break;
    case GeometryParameters::eUnknownTemperature:
      this->ui->lineEdit_nominalTemperature->setText(value);
      break;
    case GeometryParameters::eUnknownLength:
      this->ui->lineEdit_nominalLength->setText(value);
      break;
  }
}

/*!
 * \brief NominalPropertiesDialog::getParametersFromGUI
 * \param parameters
 */
void NominalPropertiesDialog::getParametersFromGUI(
    QMap<GeometryParameters, double>& parameters) {
  // check if the current nominal is a geometry
  if (!getIsGeometry(this->type)) {
    return;
  }
  GeometryTypes type = getGeometryTypeEnum(this->type);

  // get a list of unknown parameters that correspond to the current geometry
  // type
  QList<GeometryParameters> currentUnknownParameters =
      getGeometryParameters(type);

  // get parameter display config
  const ParameterDisplayConfig& dConfig =
      ModelManager::getParameterDisplayConfig();

  foreach (const GeometryParameters& param, currentUnknownParameters) {
    // get values from GUI elements (only use values that differ from the
    // initial input)
    switch (param) {
      case GeometryParameters::eUnknownX:
        if (this->parameters.contains(param) &&
            this->parameters.value(param).compare(
                this->ui->lineEdit_nominalX->text()) == 0) {
          break;
        }
        parameters.insert(
            param,
            convertToDefault(this->ui->lineEdit_nominalX->text().toDouble(),
                             dConfig.getDisplayUnit(DimensionType::eMetric)));
        break;
      case GeometryParameters::eUnknownY:
        if (this->parameters.contains(param) &&
            this->parameters.value(param).compare(
                this->ui->lineEdit_nominalY->text()) == 0) {
          break;
        }
        parameters.insert(
            param,
            convertToDefault(this->ui->lineEdit_nominalY->text().toDouble(),
                             dConfig.getDisplayUnit(DimensionType::eMetric)));
        break;
      case GeometryParameters::eUnknownZ:
        if (this->parameters.contains(param) &&
            this->parameters.value(param).compare(
                this->ui->lineEdit_nominalZ->text()) == 0) {
          break;
        }
        parameters.insert(
            param,
            convertToDefault(this->ui->lineEdit_nominalZ->text().toDouble(),
                             dConfig.getDisplayUnit(DimensionType::eMetric)));
        break;
      case GeometryParameters::eUnknownPrimaryI:
        if (this->parameters.contains(param) &&
            this->parameters.value(param).compare(
                this->ui->lineEdit_nominalI1->text()) == 0) {
          break;
        }
        parameters.insert(param,
                          this->ui->lineEdit_nominalI1->text().toDouble());
        break;
      case GeometryParameters::eUnknownPrimaryJ:
        if (this->parameters.contains(param) &&
            this->parameters.value(param).compare(
                this->ui->lineEdit_nominalJ1->text()) == 0) {
          break;
        }
        parameters.insert(param,
                          this->ui->lineEdit_nominalJ1->text().toDouble());
        break;
      case GeometryParameters::eUnknownPrimaryK:
        if (this->parameters.contains(param) &&
            this->parameters.value(param).compare(
                this->ui->lineEdit_nominalK1->text()) == 0) {
          break;
        }
        parameters.insert(param,
                          this->ui->lineEdit_nominalK1->text().toDouble());
        break;
      case GeometryParameters::eUnknownSecondaryI:
        if (this->parameters.contains(param) &&
            this->parameters.value(param).compare(
                this->ui->lineEdit_nominalI2->text()) == 0) {
          break;
        }
        parameters.insert(param,
                          this->ui->lineEdit_nominalI2->text().toDouble());
        break;
      case GeometryParameters::eUnknownSecondaryJ:
        if (this->parameters.contains(param) &&
            this->parameters.value(param).compare(
                this->ui->lineEdit_nominalJ2->text()) == 0) {
          break;
        }
        parameters.insert(param,
                          this->ui->lineEdit_nominalJ2->text().toDouble());
        break;
      case GeometryParameters::eUnknownSecondaryK:
        if (this->parameters.contains(param) &&
            this->parameters.value(param).compare(
                this->ui->lineEdit_nominalK2->text()) == 0) {
          break;
        }
        parameters.insert(param,
                          this->ui->lineEdit_nominalK2->text().toDouble());
        break;
      case GeometryParameters::eUnknownRadiusA:
        if (this->parameters.contains(param) &&
            this->parameters.value(param).compare(
                this->ui->lineEdit_nominalR1->text()) == 0) {
          break;
        }
        parameters.insert(
            param,
            convertToDefault(this->ui->lineEdit_nominalR1->text().toDouble(),
                             dConfig.getDisplayUnit(DimensionType::eMetric)));
        break;
      case GeometryParameters::eUnknownRadiusB:
        if (this->parameters.contains(param) &&
            this->parameters.value(param).compare(
                this->ui->lineEdit_nominalR2->text()) == 0) {
          break;
        }
        parameters.insert(
            param,
            convertToDefault(this->ui->lineEdit_nominalR2->text().toDouble(),
                             dConfig.getDisplayUnit(DimensionType::eMetric)));
        break;
      case GeometryParameters::eUnknownAperture:
        if (this->parameters.contains(param) &&
            this->parameters.value(param).compare(
                this->ui->lineEdit_nominalAperture->text()) == 0) {
          break;
        }
        parameters.insert(
            param, convertToDefault(
                       this->ui->lineEdit_nominalAperture->text().toDouble(),
                       dConfig.getDisplayUnit(DimensionType::eAngular)));
        break;
      case GeometryParameters::eUnknownA:
        if (this->parameters.contains(param) &&
            this->parameters.value(param).compare(
                this->ui->lineEdit_nominalA->text()) == 0) {
          break;
        }
        parameters.insert(
            param,
            convertToDefault(this->ui->lineEdit_nominalA->text().toDouble(),
                             dConfig.getDisplayUnit(DimensionType::eMetric)));
        break;
      case GeometryParameters::eUnknownB:
        if (this->parameters.contains(param) &&
            this->parameters.value(param).compare(
                this->ui->lineEdit_nominalB->text()) == 0) {
          break;
        }
        parameters.insert(
            param,
            convertToDefault(this->ui->lineEdit_nominalB->text().toDouble(),
                             dConfig.getDisplayUnit(DimensionType::eMetric)));
        break;
      case GeometryParameters::eUnknownC:
        if (this->parameters.contains(param) &&
            this->parameters.value(param).compare(
                this->ui->lineEdit_nominalC->text()) == 0) {
          break;
        }
        parameters.insert(
            param,
            convertToDefault(this->ui->lineEdit_nominalC->text().toDouble(),
                             dConfig.getDisplayUnit(DimensionType::eMetric)));
        break;
      case GeometryParameters::eUnknownAngle:
        if (this->parameters.contains(param) &&
            this->parameters.value(param).compare(
                this->ui->lineEdit_nominalAngle->text()) == 0) {
          break;
        }
        parameters.insert(
            param,
            convertToDefault(this->ui->lineEdit_nominalAngle->text().toDouble(),
                             dConfig.getDisplayUnit(DimensionType::eAngular)));
        break;
      case GeometryParameters::eUnknownDistance:
        if (this->parameters.contains(param) &&
            this->parameters.value(param).compare(
                this->ui->lineEdit_nominalDistance->text()) == 0) {
          break;
        }
        parameters.insert(
            param, convertToDefault(
                       this->ui->lineEdit_nominalDistance->text().toDouble(),
                       dConfig.getDisplayUnit(DimensionType::eMetric)));
        break;
      case GeometryParameters::eUnknownMeasurementSeries:
        if (this->parameters.contains(param) &&
            this->parameters.value(param).compare(
                this->ui->lineEdit_nominalMeasurementSeries->text()) == 0) {
          break;
        }
        parameters.insert(
            param,
            convertToDefault(
                this->ui->lineEdit_nominalMeasurementSeries->text().toDouble(),
                dConfig.getDisplayUnit(DimensionType::eMetric)));
        break;
      case GeometryParameters::eUnknownTemperature:
        if (this->parameters.contains(param) &&
            this->parameters.value(param).compare(
                this->ui->lineEdit_nominalTemperature->text()) == 0) {
          break;
        }
        parameters.insert(
            param, convertToDefault(
                       this->ui->lineEdit_nominalTemperature->text().toDouble(),
                       dConfig.getDisplayUnit(DimensionType::eTemperature)));
        break;
      case GeometryParameters::eUnknownLength:
        if (this->parameters.contains(param) &&
            this->parameters.value(param).compare(
                this->ui->lineEdit_nominalLength->text()) == 0) {
          break;
        }
        parameters.insert(
            param, convertToDefault(
                       this->ui->lineEdit_nominalLength->text().toDouble(),
                       dConfig.getDisplayUnit(DimensionType::eMetric)));
        break;
    }
  }
}

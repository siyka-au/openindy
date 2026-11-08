#include "scalar_parameter_widget.h"

#include <map>

#include "containers.h"
#include "controller/qt_convert.h"

ScalarParameterWidget::ScalarParameterWidget(QWidget* parent)
    : QWidget(parent),
      doubleValidator(new QRegularExpressionValidator(this->validDouble)),
      intValidator(new QRegularExpressionValidator(this->validInt)),
      myLayout(nullptr) {
  // set up regular expressions
  this->validInt = QRegularExpression("^-?(0|[1-9]{1}[0-9]{0,9})$");
  this->validDouble =
      QRegularExpression("^-?(0|[1-9]{1}[0-9]{0,9})(\\.[0-9]+)?$");

  this->setUpGui();
}

/*!
 * \brief ScalarParameterWidget::setIntParameter
 * \param intParameter
 */
void ScalarParameterWidget::setIntParameter(
    const std::map<QString, int>& intParameter) {
  this->intParameter = intParameter;
  this->setUpGui();
}

/*!
 * \brief ScalarParameterWidget::setDoubleParameter
 * \param doubleParameter
 */
void ScalarParameterWidget::setDoubleParameter(
    const std::map<QString, double>& doubleParameter) {
  this->doubleParameter = doubleParameter;
  this->setUpGui();
}

/*!
 * \brief ScalarParameterWidget::setStringParameter
 * \param stringParameter
 * \param defaultSelection
 */
void ScalarParameterWidget::setStringParameter(
    const std::multimap<QString, QString>& stringParameter,
    const std::map<QString, QString>& defaultSelection) {
  this->stringParameter.clear();
  foreach (const QString& key, oi::keys(stringParameter)) {
    QStringList values;
    for (const QString& value : oi::values(stringParameter, key)) {
      values.append(value);
    }
    this->stringParameter.insert(key, values);
  }
  this->stringParameterSelection = defaultSelection;
  this->setUpGui();
}

/*!
 * \brief ScalarParameterWidget::getIntParameter
 * \return
 */
const std::map<QString, int>& ScalarParameterWidget::getIntParameter() {
  this->getParameterFromGUI();
  return this->intParameter;
}

/*!
 * \brief ScalarParameterWidget::getDoubleParameter
 * \return
 */
const std::map<QString, double>& ScalarParameterWidget::getDoubleParameter() {
  this->getParameterFromGUI();
  return this->doubleParameter;
}

/*!
 * \brief ScalarParameterWidget::getStringParameter
 * \return
 */
const std::map<QString, QString>& ScalarParameterWidget::getStringParameter() {
  this->getParameterFromGUI();
  return this->stringParameterSelection;
}

/*!
 * \brief ScalarParameterWidget::clearAll
 */
void ScalarParameterWidget::clearAll() {
  // clear user defined parameters
  this->intParameter.clear();
  this->doubleParameter.clear();
  this->stringParameter.clear();
  this->stringParameterSelection.clear();

  // clear GUI elements
  this->clearGuiElements();
}

/*!
 * \brief ScalarParameterWidget::setUpGui
 * Set up the GUI with all custom parameters
 */
void ScalarParameterWidget::setUpGui() {
  // delete old GUI elements
  this->clearGuiElements();

  // get iterators for parameter maps
  QMapIterator<QString, QStringList> stringIterator(this->stringParameter);

  // set up layout
  if (this->myLayout != nullptr) {
    delete this->myLayout;
  }
  this->myLayout = new QFormLayout();

  // create GUI elements for each parameter
  QFont boldFont;
  boldFont.setBold(true);
  int rowIndex = 0;
  while (stringIterator.hasNext()) {
    stringIterator.next();
    QString stringKey = static_cast<QString>(stringIterator.key());
    QStringList stringValue = static_cast<QStringList>(stringIterator.value());
    QLabel* stringLabel = new QLabel(stringKey);
    stringLabel->setMinimumWidth(300);
    stringLabel->setWordWrap(true);
    stringLabel->setFont(boldFont);
    QComboBox* stringComboBox = new QComboBox();
    stringComboBox->addItems(stringValue);
    stringComboBox->setCurrentText(
        oi::value(this->stringParameterSelection, stringKey));
    this->parameterLabel.append(stringLabel);
    this->stringParameterComboBox.insert(stringKey, stringComboBox);
    this->myLayout->insertRow(rowIndex, stringLabel, stringComboBox);
    QObject::connect(stringComboBox, &QComboBox::currentTextChanged, this,
                     &ScalarParameterWidget::scalarParametersChanged,
                     Qt::AutoConnection);
    rowIndex++;
  }
  for (const auto& [intKey, intValue] : this->intParameter) {
    QLabel* intLabel = new QLabel(intKey);
    intLabel->setMinimumWidth(300);
    intLabel->setWordWrap(true);
    intLabel->setFont(boldFont);
    QLineEdit* intLineEdit = new QLineEdit(QString::number(intValue));
    intLineEdit->setValidator(this->intValidator);
    this->parameterLabel.append(intLabel);
    this->intParameterLineEdit.insert(intKey, intLineEdit);
    this->myLayout->insertRow(rowIndex, intLabel, intLineEdit);
    QObject::connect(intLineEdit, &QLineEdit::textChanged, this,
                     &ScalarParameterWidget::scalarParametersChanged,
                     Qt::AutoConnection);
    rowIndex++;
  }
  for (const auto& [doubleKey, doubleValue] : this->doubleParameter) {
    QLabel* doubleLabel = new QLabel(doubleKey);
    doubleLabel->setMinimumWidth(300);
    doubleLabel->setWordWrap(true);
    doubleLabel->setFont(boldFont);
    QLineEdit* doubleLineEdit = new QLineEdit(QString::number(doubleValue));
    doubleLineEdit->setValidator(this->doubleValidator);
    this->parameterLabel.append(doubleLabel);
    this->doubleParameterLineEdit.insert(doubleKey, doubleLineEdit);
    this->myLayout->insertRow(rowIndex, doubleLabel, doubleLineEdit);
    QObject::connect(doubleLineEdit, &QLineEdit::textChanged, this,
                     &ScalarParameterWidget::scalarParametersChanged,
                     Qt::AutoConnection);
    rowIndex++;
  }

  // assign layout
  this->setLayout(this->myLayout);
}

/*!
 * \brief ScalarParameterWidget::clearGuiElements
 * Clears all GUI elements in this widget
 */
void ScalarParameterWidget::clearGuiElements() {
  // get iterator for maps with GUI elements
  QMapIterator<QString, QLineEdit*> intIterator(this->intParameterLineEdit);
  QMapIterator<QString, QLineEdit*> doubleIterator(
      this->doubleParameterLineEdit);
  QMapIterator<QString, QComboBox*> stringIterator(
      this->stringParameterComboBox);

  // delete GUI elements
  while (intIterator.hasNext()) {
    delete static_cast<QLineEdit*>(intIterator.next().value());
  }
  while (doubleIterator.hasNext()) {
    delete static_cast<QLineEdit*>(doubleIterator.next().value());
  }
  while (stringIterator.hasNext()) {
    delete static_cast<QComboBox*>(stringIterator.next().value());
  }
  foreach (QLabel* label, this->parameterLabel) {
    delete label;
  }

  // clear containers
  this->parameterLabel.clear();
  this->intParameterLineEdit.clear();
  this->doubleParameterLineEdit.clear();
  this->stringParameterComboBox.clear();
}

/*!
 * \brief ScalarParameterWidget::getParameterFromGUI
 * Set up user defined parameters from GUI elements
 */
void ScalarParameterWidget::getParameterFromGUI() {
  // set up int parameters
  QMapIterator<QString, QLineEdit*> intIterator(this->intParameterLineEdit);
  while (intIterator.hasNext()) {
    intIterator.next();
    QString intKey = static_cast<QString>(intIterator.key());
    QLineEdit* intValue = static_cast<QLineEdit*>(intIterator.value());
    if (intValue->hasAcceptableInput()) {  // if line edit contains an integer
      this->intParameter.insert_or_assign(intKey, intValue->text().toInt());
    } else if (intValue->text().trimmed().isEmpty()) {
      this->intParameter.insert_or_assign(intKey, 0);  // set to 0
    }
  }

  // set up double parameters
  QMapIterator<QString, QLineEdit*> doubleIterator(
      this->doubleParameterLineEdit);
  while (doubleIterator.hasNext()) {
    doubleIterator.next();
    QString doubleKey = static_cast<QString>(doubleIterator.key());
    QLineEdit* doubleValue = static_cast<QLineEdit*>(doubleIterator.value());
    if (doubleValue->hasAcceptableInput()) {  // if line edit contains a double
      this->doubleParameter.insert_or_assign(doubleKey,
                                             doubleValue->text().toDouble());
    } else if (doubleValue->text().trimmed().isEmpty()) {
      this->doubleParameter.insert_or_assign(doubleKey, 0.0);  // set to 0
    }
  }

  // set up string parameters
  QMapIterator<QString, QComboBox*> stringIterator(
      this->stringParameterComboBox);
  while (stringIterator.hasNext()) {
    stringIterator.next();
    QString stringKey = static_cast<QString>(stringIterator.key());
    QComboBox* stringValue = static_cast<QComboBox*>(stringIterator.value());
    this->stringParameterSelection.insert_or_assign(stringKey,
                                                    stringValue->currentText());
  }
}

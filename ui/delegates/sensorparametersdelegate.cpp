#include "sensorparametersdelegate.h"

#include <QComboBox>
#include <iterator>
#include <utility>

#include "containers.h"
#include "controller/qtconvert.h"

/*!
 * \brief SensorParametersDelegate::SensorParametersDelegate
 * \param parent
 */
SensorParametersDelegate::SensorParametersDelegate(QObject* parent)
    : QItemDelegate(parent) {}

/*!
 * \brief SensorParametersDelegate::createEditor
 * Create the editor widget
 * \param parent
 * \param option
 * \param index
 */
QWidget* SensorParametersDelegate::createEditor(
    QWidget* parent, const QStyleOptionViewItem& option,
    const QModelIndex& index) const {
  QWidget* editor = nullptr;

  // check model index
  if (!index.isValid()) {
    return editor;
  }

  // get and check model
  const SensorParametersModel* model =
      static_cast<const SensorParametersModel*>(index.model());
  if (model == nullptr) {
    return editor;
  }

  // get sensor config
  const SensorConfiguration& sConfig = model->getSensorConfiguration();

  // get row and column indices
  int rowIndex = index.row();
  int columnIndex = index.column();

  // create editor
  if (rowIndex < std::ssize(oi::keys(sConfig.getStringParameter()))) {
    // create editor with model
    editor = new QComboBox(parent);
    QStringListModel* model = new QStringListModel(
        toQStringList(
            oi::values(sConfig.getAvailableStringParameter(),
                       oi::keys(sConfig.getStringParameter()).at(rowIndex))),
        editor);
    static_cast<QComboBox*>(editor)->setModel(model);

  } else if (std::cmp_less(rowIndex, sConfig.getDoubleParameter().size() +
                                         std::ssize(oi::keys(
                                             sConfig.getStringParameter())))) {
    rowIndex = rowIndex - std::ssize(oi::keys(sConfig.getStringParameter()));
    editor = new QLineEdit(parent);
  } else {
    rowIndex = rowIndex - std::ssize(oi::keys(sConfig.getStringParameter())) -
               sConfig.getDoubleParameter().size();
    editor = new QLineEdit(parent);
  }

  return editor;
}

/*!
 * \brief SensorParametersDelegate::setEditorData
 * Set current input of editor widget
 * \param editor
 * \param index
 */
void SensorParametersDelegate::setEditorData(QWidget* editor,
                                             const QModelIndex& index) const {
  // check model index
  if (!index.isValid()) {
    return;
  }

  // get and check model
  const SensorParametersModel* model =
      static_cast<const SensorParametersModel*>(index.model());
  if (model == nullptr) {
    return;
  }

  // get sensor config
  const SensorConfiguration& sConfig = model->getSensorConfiguration();

  // get row and column indices
  int rowIndex = index.row();
  int columnIndex = index.column();

  // set editor default input
  if (rowIndex < std::ssize(oi::keys(sConfig.getStringParameter()))) {
    QComboBox* customEditor = nullptr;
    customEditor = qobject_cast<QComboBox*>(editor);
    if (customEditor != nullptr) {
      customEditor->setCurrentText(index.data().toString());
    }

  } else if (std::cmp_less(rowIndex, sConfig.getDoubleParameter().size() +
                                         std::ssize(oi::keys(
                                             sConfig.getStringParameter())))) {
    rowIndex = rowIndex - std::ssize(oi::keys(sConfig.getStringParameter()));
    QLineEdit* customEditor = nullptr;
    customEditor = qobject_cast<QLineEdit*>(editor);
    if (customEditor != nullptr) {
      customEditor->setText(index.data().toString());
    }

  } else {
    rowIndex = rowIndex - std::ssize(oi::keys(sConfig.getStringParameter())) -
               sConfig.getDoubleParameter().size();
    QLineEdit* customEditor = nullptr;
    customEditor = qobject_cast<QLineEdit*>(editor);
    if (customEditor != nullptr) {
      customEditor->setText(index.data().toString());
    }
  }
}

/*!
 * \brief SensorParametersDelegate::setModelData
 * Commit the edited input of the editor widget back to the model
 * \param editor
 * \param model
 * \param index
 */
void SensorParametersDelegate::setModelData(QWidget* editor,
                                            QAbstractItemModel* model,
                                            const QModelIndex& index) const {
  // check model index
  if (!index.isValid()) {
    return;
  }

  // get and check model
  const SensorParametersModel* sensorParamModel =
      static_cast<const SensorParametersModel*>(index.model());
  if (sensorParamModel == nullptr) {
    return;
  }

  // get sensor config
  const SensorConfiguration& sConfig =
      sensorParamModel->getSensorConfiguration();

  // get row and column indices
  int rowIndex = index.row();
  int columnIndex = index.column();

  // commit changes
  if (rowIndex < std::ssize(oi::keys(sConfig.getStringParameter()))) {
    QComboBox* customEditor = qobject_cast<QComboBox*>(editor);
    if (customEditor != nullptr) {
      model->setData(index, customEditor->currentText());
    }

  } else if (std::cmp_less(rowIndex, sConfig.getDoubleParameter().size() +
                                         std::ssize(oi::keys(
                                             sConfig.getStringParameter())))) {
    rowIndex = rowIndex - std::ssize(oi::keys(sConfig.getStringParameter()));
    QLineEdit* customEditor = qobject_cast<QLineEdit*>(editor);
    if (customEditor != nullptr) {
      model->setData(index, customEditor->text());
    }

  } else {
    rowIndex = rowIndex - std::ssize(oi::keys(sConfig.getStringParameter())) -
               sConfig.getDoubleParameter().size();
    QLineEdit* customEditor = qobject_cast<QLineEdit*>(editor);
    if (customEditor != nullptr) {
      model->setData(index, customEditor->text());
    }
  }
}

#include "sensor_accuracy_delegate.h"

/*!
 * \brief SensorAccuracyDelegate::SensorAccuracyDelegate
 * \param parent
 */
SensorAccuracyDelegate::SensorAccuracyDelegate(QObject* parent)
    : QItemDelegate(parent) {}

/*!
 * \brief SensorAccuracyDelegate::createEditor
 * Create the editor widget
 * \param parent
 * \param option
 * \param index
 */
QWidget* SensorAccuracyDelegate::createEditor(
    QWidget* parent, const QStyleOptionViewItem& option,
    const QModelIndex& index) const {
  // create line edit
  QLineEdit* editor = nullptr;

  // check model index
  if (!index.isValid()) {
    return editor;
  }

  // get and check model
  const SensorAccuracyModel* model =
      static_cast<const SensorAccuracyModel*>(index.model());
  if (model == nullptr) {
    return editor;
  }

  // get row and column indices
  int rowIndex = index.row();
  int columnIndex = index.column();

  // sigma type headers
  if (rowIndex == SensorAccuracyRows::kRowPolarHeader ||
      rowIndex == SensorAccuracyRows::kRowCartesianHeader ||
      rowIndex == SensorAccuracyRows::kRowLevelHeader ||
      rowIndex == SensorAccuracyRows::kRowTemperatureHeader ||
      rowIndex == SensorAccuracyRows::kRowUndefinedHeader) {
    return editor;
  }

  // create editor
  editor = new QLineEdit(parent);

  return editor;
}

/*!
 * \brief SensorAccuracyDelegate::setEditorData
 * Set current input of editor widget
 * \param editor
 * \param index
 */
void SensorAccuracyDelegate::setEditorData(QWidget* editor,
                                           const QModelIndex& index) const {
  // check model index
  if (!index.isValid()) {
    return;
  }

  // get and check model
  const SensorAccuracyModel* model =
      static_cast<const SensorAccuracyModel*>(index.model());
  if (model == nullptr) {
    return;
  }

  // get row and column indices
  int rowIndex = index.row();
  int columnIndex = index.column();

  // sigma type headers
  if (rowIndex == SensorAccuracyRows::kRowPolarHeader ||
      rowIndex == SensorAccuracyRows::kRowCartesianHeader ||
      rowIndex == SensorAccuracyRows::kRowLevelHeader ||
      rowIndex == SensorAccuracyRows::kRowTemperatureHeader ||
      rowIndex == SensorAccuracyRows::kRowUndefinedHeader) {
    return;
  }

  // set editor default input
  QLineEdit* customEditor = nullptr;
  customEditor = qobject_cast<QLineEdit*>(editor);
  if (customEditor != nullptr) {
    customEditor->setText(index.data().toString());
  }
}

/*!
 * \brief SensorAccuracyDelegate::setModelData
 * Commit the edited input of the editor widget back to the model
 * \param editor
 * \param model
 * \param index
 */
void SensorAccuracyDelegate::setModelData(QWidget* editor,
                                          QAbstractItemModel* model,
                                          const QModelIndex& index) const {
  // check model index
  if (!index.isValid()) {
    return;
  }

  // get row and column indices
  int rowIndex = index.row();
  int columnIndex = index.column();

  // sigma type headers
  if (rowIndex == SensorAccuracyRows::kRowPolarHeader ||
      rowIndex == SensorAccuracyRows::kRowCartesianHeader ||
      rowIndex == SensorAccuracyRows::kRowLevelHeader ||
      rowIndex == SensorAccuracyRows::kRowTemperatureHeader ||
      rowIndex == SensorAccuracyRows::kRowUndefinedHeader) {
    return;
  }

  // commit changes
  QLineEdit* customEditor = qobject_cast<QLineEdit*>(editor);
  if (customEditor != nullptr) {
    model->setData(index, customEditor->text());
  }
}

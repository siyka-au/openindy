#ifndef OI_APP_UI_DELEGATES_SENSOR_ACCURACY_DELEGATE_H_
#define OI_APP_UI_DELEGATES_SENSOR_ACCURACY_DELEGATE_H_

#include <QAbstractTableModel>
#include <QItemDelegate>
#include <QLineEdit>
#include <QModelIndex>

#include "sensor_accuracy_model.h"

/*!
 * \brief The SensorAccuracyDelegate class
 * Delegate to edit data in SensorAccuracyModel
 */
class SensorAccuracyDelegate : public QItemDelegate {
 public:
  explicit SensorAccuracyDelegate(QObject* parent = nullptr);

  QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option,
                        const QModelIndex& index) const override;
  void setEditorData(QWidget* editor, const QModelIndex& index) const override;
  void setModelData(QWidget* editor, QAbstractItemModel* model,
                    const QModelIndex& index) const override;
};

#endif  // OI_APP_UI_DELEGATES_SENSOR_ACCURACY_DELEGATE_H_

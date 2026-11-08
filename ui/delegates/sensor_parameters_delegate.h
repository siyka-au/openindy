#ifndef OI_APP_UI_DELEGATES_SENSOR_PARAMETERS_DELEGATE_H_
#define OI_APP_UI_DELEGATES_SENSOR_PARAMETERS_DELEGATE_H_

#include <QAbstractTableModel>
#include <QItemDelegate>
#include <QLineEdit>
#include <QModelIndex>

#include "model_manager.h"
#include "sensor_parameters_model.h"

/*!
 * \brief The SensorParametersDelegate class
 * Delegate to edit data in SensorParametersModel
 */
class SensorParametersDelegate : public QItemDelegate {
 public:
  explicit SensorParametersDelegate(QObject* parent = nullptr);

  QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option,
                        const QModelIndex& index) const override;
  void setEditorData(QWidget* editor, const QModelIndex& index) const override;
  void setModelData(QWidget* editor, QAbstractItemModel* model,
                    const QModelIndex& index) const override;
};

#endif  // OI_APP_UI_DELEGATES_SENSOR_PARAMETERS_DELEGATE_H_

#ifndef OI_APP_UI_DELEGATES_SENSOR_CONFIGURATION_LIST_DELEGATE_H_
#define OI_APP_UI_DELEGATES_SENSOR_CONFIGURATION_LIST_DELEGATE_H_

#include <QAbstractTableModel>
#include <QItemDelegate>
#include <QLineEdit>
#include <QModelIndex>

#include "sensor_configuration_model.h"
#include "sensor_configuration_proxy_model.h"

using namespace oi;

/*!
 * \brief The SensorConfigurationListDelegate class
 * Delegate to edit data in SensorConfigurationModel
 */
class SensorConfigurationListDelegate : public QItemDelegate {
 public:
  explicit SensorConfigurationListDelegate(QObject* parent = nullptr);

  QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option,
                        const QModelIndex& index) const override;
  void setEditorData(QWidget* editor, const QModelIndex& index) const override;
  void setModelData(QWidget* editor, QAbstractItemModel* model,
                    const QModelIndex& index) const override;
};

#endif  // OI_APP_UI_DELEGATES_SENSOR_CONFIGURATION_LIST_DELEGATE_H_

#ifndef SENSORPARAMETERSDELEGATE_H
#define SENSORPARAMETERSDELEGATE_H

#include <QAbstractTableModel>
#include <QItemDelegate>
#include <QLineEdit>
#include <QModelIndex>

#include "modelmanager.h"
#include "sensorparametersmodel.h"

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

#endif  // SENSORPARAMETERSDELEGATE_H

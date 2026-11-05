#ifndef SENSORACCURACYDELEGATE_H
#define SENSORACCURACYDELEGATE_H

#include <QAbstractTableModel>
#include <QItemDelegate>
#include <QLineEdit>
#include <QModelIndex>

#include "sensoraccuracymodel.h"

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

#endif  // SENSORACCURACYDELEGATE_H

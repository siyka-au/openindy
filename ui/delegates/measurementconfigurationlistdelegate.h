#ifndef MEASUREMENTCONFIGURATIONDELEGATE_H
#define MEASUREMENTCONFIGURATIONDELEGATE_H

#include <QAbstractTableModel>
#include <QItemDelegate>
#include <QLineEdit>
#include <QModelIndex>

#include "measurementconfigurationmodel.h"
#include "measurementconfigurationproxymodel.h"

using namespace oi;

/*!
 * \brief The MeasurementConfigurationListDelegate class
 * Delegate to edit data in MeasurementConfigurationModel
 */
class MeasurementConfigurationListDelegate : public QItemDelegate {
 public:
  MeasurementConfigurationListDelegate(QObject* parent = nullptr);

  QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option,
                        const QModelIndex& index) const override;
  void setEditorData(QWidget* editor, const QModelIndex& index) const override;
  void setModelData(QWidget* editor, QAbstractItemModel* model,
                    const QModelIndex& index) const override;
};

#endif  // MEASUREMENTCONFIGURATIONDELEGATE_H

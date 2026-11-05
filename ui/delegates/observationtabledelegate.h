#ifndef OBSERVATIONTABLEDELEGATE_H
#define OBSERVATIONTABLEDELEGATE_H

#include <QAbstractTableModel>
#include <QItemDelegate>
#include <QLineEdit>
#include <QModelIndex>

#include "observationmodel.h"
#include "observationproxymodel.h"

using namespace oi;

/*!
 * \brief The ObservationTableDelegate class
 */
class ObservationTableDelegate : public QItemDelegate {
 public:
  explicit ObservationTableDelegate(QObject* parent = nullptr);

  QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option,
                        const QModelIndex& index) const override;
  void setEditorData(QWidget* editor, const QModelIndex& index) const override;
  void setModelData(QWidget* editor, QAbstractItemModel* model,
                    const QModelIndex& index) const override;
};

#endif  // OBSERVATIONTABLEDELEGATE_H

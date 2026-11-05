#ifndef READINGTABLEDELEGATE_H
#define READINGTABLEDELEGATE_H

#include <QAbstractTableModel>
#include <QItemDelegate>
#include <QLineEdit>
#include <QModelIndex>

#include "readingmodel.h"
#include "readingproxymodel.h"

using namespace oi;

/*!
 * \brief The ReadingTableDelegate class
 */
class ReadingTableDelegate : public QItemDelegate {
 public:
  ReadingTableDelegate(QObject* parent = 0);

  QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option,
                        const QModelIndex& index) const;
  void setEditorData(QWidget* editor, const QModelIndex& index) const;
  void setModelData(QWidget* editor, QAbstractItemModel* model,
                    const QModelIndex& index) const;
};

#endif  // READINGTABLEDELEGATE_H

#ifndef FUNCTIONWEIGHTDELEGATE_H
#define FUNCTIONWEIGHTDELEGATE_H

#include <QAbstractTableModel>
#include <QCheckBox>
#include <QItemDelegate>
#include <QLineEdit>
#include <QModelIndex>

/*!
 * \brief The FunctionWeightDelegate class
 * to make the checkboxes set the values to ignore or use parameters
 */
class FunctionWeightDelegate : public QItemDelegate {
 public:
  FunctionWeightDelegate(QObject* parent = 0);

  QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option,
                        const QModelIndex& index) const;
  void setEditorData(QWidget* editor, const QModelIndex& index) const;
  void setModelData(QWidget* editor, QAbstractItemModel* model,
                    const QModelIndex& index) const;
};

#endif  // FUNCTIONWEIGHTDELEGATE_H

#ifndef TRAFOPARAMDELEGATE_H
#define TRAFOPARAMDELEGATE_H

#include <QCheckBox>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QItemDelegate>
#include <QLineEdit>

#include "trafoparamtableproxymodel.h"

using namespace oi;

class TrafoParamDelegate : public QItemDelegate {
 public:
  explicit TrafoParamDelegate(QObject* parent = nullptr);

  QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option,
                        const QModelIndex& index) const override;
  void setEditorData(QWidget* editor, const QModelIndex& index) const override;
  void setModelData(QWidget* editor, QAbstractItemModel* model,
                    const QModelIndex& index) const override;
};

#endif  // TRAFOPARAMDELEGATE_H

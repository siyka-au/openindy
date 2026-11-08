#ifndef OI_APP_UI_DELEGATES_TRAFO_PARAM_TABLE_DELEGATE_H_
#define OI_APP_UI_DELEGATES_TRAFO_PARAM_TABLE_DELEGATE_H_

#include <QCheckBox>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QItemDelegate>
#include <QLineEdit>

#include "trafo_param_table_proxy_model.h"

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

#endif  // OI_APP_UI_DELEGATES_TRAFO_PARAM_TABLE_DELEGATE_H_

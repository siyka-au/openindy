#ifndef OI_APP_UI_DELEGATES_FEATURE_TABLE_DELEGATE_H_
#define OI_APP_UI_DELEGATES_FEATURE_TABLE_DELEGATE_H_

#include <QAbstractTableModel>
#include <QCheckBox>
#include <QItemDelegate>
#include <QLineEdit>
#include <QModelIndex>

#include "feature_table_model.h"
#include "feature_table_proxy_model.h"

using namespace oi;

/*!
 * \brief The FeatureTableDelegate class
 * Delegate to edit data in FeatureTableModel
 */
class FeatureTableDelegate : public QItemDelegate {
 public:
  explicit FeatureTableDelegate(QObject* parent = nullptr);

  QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option,
                        const QModelIndex& index) const override;
  void setEditorData(QWidget* editor, const QModelIndex& index) const override;
  void setModelData(QWidget* editor, QAbstractItemModel* model,
                    const QModelIndex& index) const override;

  bool activeFeatureIsNominal(
      const FeatureTableProxyModel* featureTablePM) const;
};

#endif  // OI_APP_UI_DELEGATES_FEATURE_TABLE_DELEGATE_H_

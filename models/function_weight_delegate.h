#ifndef OI_APP_MODELS_FUNCTION_WEIGHT_DELEGATE_H_
#define OI_APP_MODELS_FUNCTION_WEIGHT_DELEGATE_H_

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
  explicit FunctionWeightDelegate(QObject* parent = nullptr);

  QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option,
                        const QModelIndex& index) const override;
  void setEditorData(QWidget* editor, const QModelIndex& index) const override;
  void setModelData(QWidget* editor, QAbstractItemModel* model,
                    const QModelIndex& index) const override;
};

#endif  // OI_APP_MODELS_FUNCTION_WEIGHT_DELEGATE_H_

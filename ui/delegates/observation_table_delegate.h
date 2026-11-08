#ifndef OI_APP_UI_DELEGATES_OBSERVATION_TABLE_DELEGATE_H_
#define OI_APP_UI_DELEGATES_OBSERVATION_TABLE_DELEGATE_H_

#include <QAbstractTableModel>
#include <QItemDelegate>
#include <QLineEdit>
#include <QModelIndex>

#include "observation_model.h"
#include "observation_proxy_model.h"

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

#endif  // OI_APP_UI_DELEGATES_OBSERVATION_TABLE_DELEGATE_H_

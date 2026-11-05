#ifndef FEATURETABLEDELEGATE_H
#define FEATURETABLEDELEGATE_H

#include <QAbstractTableModel>
#include <QCheckBox>
#include <QItemDelegate>
#include <QLineEdit>
#include <QModelIndex>

#include "featuretablemodel.h"
#include "featuretableproxymodel.h"

using namespace oi;

/*!
 * \brief The FeatureTableDelegate class
 * Delegate to edit data in FeatureTableModel
 */
class FeatureTableDelegate : public QItemDelegate {
 public:
  FeatureTableDelegate(QObject* parent = 0);

  QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option,
                        const QModelIndex& index) const;
  void setEditorData(QWidget* editor, const QModelIndex& index) const;
  void setModelData(QWidget* editor, QAbstractItemModel* model,
                    const QModelIndex& index) const;

  bool activeFeatureIsNominal(
      const FeatureTableProxyModel* featureTablePM) const;
};

#endif  // FEATURETABLEDELEGATE_H

#ifndef OI_APP_MODELS_TREEITEMS_FUNCTION_TREE_ITEM_H_
#define OI_APP_MODELS_TREEITEMS_FUNCTION_TREE_ITEM_H_

#include <QList>
#include <QObject>
#include <QPointer>
#include <QtCore>

#include "feature_wrapper.h"
#include "observation.h"
#include "reading.h"
#include "types.h"
#include "util.h"

using namespace oi;

/*!
 * \brief The FeatureTreeItem class
 * Class used to hold single items of the ActiveFeatureFunctionModel
 */
class FunctionTreeItem : public QObject {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(FunctionTreeItem)

 public:
  explicit FunctionTreeItem(const QVariant& display, QObject* parent = nullptr);
  ~FunctionTreeItem() override;

  // ##########################################
  // get or set information about the tree item
  // ##########################################

  // get or set children of this tree item
  [[nodiscard]] int getChildCount() const;
  void appendChild(const QPointer<FunctionTreeItem>& child);
  QPointer<FunctionTreeItem> getChild(const int& index);
  void deleteChildren();

  // get the index of this tree item in the children list of its parent
  [[nodiscard]] const int& getIndex() const;

  // get parent of this tree item
  [[nodiscard]] bool hasParent() const;
  [[nodiscard]] const QPointer<FunctionTreeItem>& getParent() const;

  // get the value to display in the tree view
  [[nodiscard]] const QVariant& getDisplayValue() const;

 private:
  // ####################################################
  // attributes that hold information about the tree item
  // ####################################################

  // parent and child items
  QPointer<FunctionTreeItem> parent;
  QList<QPointer<FunctionTreeItem> > children;

  // index of this tree item in the children list of its parent
  int index;

  // number of children of this tree item
  int childCount;

  // value that is displayed in tree view for this item
  QVariant displayValue;
};

#endif  // OI_APP_MODELS_TREEITEMS_FUNCTION_TREE_ITEM_H_

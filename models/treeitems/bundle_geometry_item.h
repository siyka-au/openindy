#ifndef OI_APP_MODELS_TREEITEMS_BUNDLE_GEOMETRY_ITEM_H_
#define OI_APP_MODELS_TREEITEMS_BUNDLE_GEOMETRY_ITEM_H_

#include <QVariant>
#include <vector>

/*!
 * \brief The BundleGeometryItem class
 */
class BundleGeometryItem {
 public:
  explicit BundleGeometryItem(const QString& key,
                              BundleGeometryItem* parentItem = nullptr);
  ~BundleGeometryItem();

  // ######################
  // tree view item methods
  // ######################

  // children
  void appendChild(BundleGeometryItem* child);
  void removeAllChildren();
  BundleGeometryItem* child(int row);
  [[nodiscard]] int childCount() const;

  // row and column count
  [[nodiscard]] int columnCount() const;
  [[nodiscard]] QVariant data(int column) const;
  [[nodiscard]] int row() const;

  // parent
  BundleGeometryItem* parentItem();
  void setParentItem(BundleGeometryItem* parent);

  // #################
  // getter and setter
  // #################

  // key
  [[nodiscard]] const QString& getKey() const;

 private:
  // ##########
  // attributes
  // ##########

  // display values
  QString key;

  // children and parent
  std::vector<BundleGeometryItem*> m_childItems;
  BundleGeometryItem* m_parentItem;
};

#endif  // OI_APP_MODELS_TREEITEMS_BUNDLE_GEOMETRY_ITEM_H_

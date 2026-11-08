#include "bundle_geometry_item.h"

#include <utility>

#include "containers.h"

/*!
 * \brief BundleGeometryItem::BundleGeometryItem
 * \param key
 * \param parentItem
 */
BundleGeometryItem::BundleGeometryItem(const QString& key,
                                       BundleGeometryItem* parentItem)
    : m_parentItem(parentItem), key(key) {
  // set parent

  // set display values
}

/*!
 * \brief BundleGeometryItem::~BundleGeometryItem
 */
BundleGeometryItem::~BundleGeometryItem() {
  // delete all child items
  qDeleteAll(this->m_childItems);
}

/*!
 * \brief BundleGeometryItem::appendChild
 * \param child
 */
void BundleGeometryItem::appendChild(BundleGeometryItem* child) {
  this->m_childItems.push_back(child);
  child->setParentItem(this);
}

/*!
 * \brief BundleGeometryItem::removeAllChildren
 */
void BundleGeometryItem::removeAllChildren() {
  qDeleteAll(this->m_childItems);
  this->m_childItems.clear();
}

/*!
 * \brief BundleGeometryItem::child
 * \param row
 * \return
 */
BundleGeometryItem* BundleGeometryItem::child(int row) {
  if (std::cmp_greater(this->m_childItems.size(), row)) {
    return oi::value(this->m_childItems, row);
  }
  return nullptr;
}

/*!
 * \brief BundleGeometryItem::childCount
 * \return
 */
int BundleGeometryItem::childCount() const { return m_childItems.size(); }

/*!
 * \brief BundleGeometryItem::columnCount
 * \return
 */
int BundleGeometryItem::columnCount() const { return 1; }

/*!
 * \brief BundleGeometryItem::data
 * \param column
 * \return
 */
QVariant BundleGeometryItem::data(int column) const {
  switch (column) {
    case 0:
      return this->key;
    default:
      return QVariant();
  }
}

/*!
 * \brief BundleGeometryItem::row
 * \return
 */
int BundleGeometryItem::row() const {
  // check parent
  if (this->m_parentItem == nullptr) {
    return 0;
  }

  return oi::indexOf(this->m_parentItem->m_childItems,
                     const_cast<BundleGeometryItem*>(this));
}

/*!
 * \brief BundleGeometryItem::parentItem
 * \return
 */
BundleGeometryItem* BundleGeometryItem::parentItem() {
  return this->m_parentItem;
}

/*!
 * \brief BundleGeometryItem::setParentItem
 * \param parent
 */
void BundleGeometryItem::setParentItem(BundleGeometryItem* parent) {
  this->m_parentItem = parent;
}

/*!
 * \brief BundleGeometryItem::getKey
 * \return
 */
const QString& BundleGeometryItem::getKey() const { return this->key; }

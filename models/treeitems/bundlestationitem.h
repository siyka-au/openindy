#ifndef BUNDLESTATIONITEM_H
#define BUNDLESTATIONITEM_H

#include <QVariant>

/*!
 * \brief The BundleStationItem class
 */
class BundleStationItem {
 public:
  BundleStationItem(const QString& key, const bool& value, const int& stationId,
                    BundleStationItem* parentItem = nullptr);
  ~BundleStationItem();

  // ######################
  // tree view item methods
  // ######################

  // children
  void appendChild(BundleStationItem* child);
  void removeAllChildren();
  BundleStationItem* child(int row);
  [[nodiscard]] int childCount() const;

  // row and column count
  [[nodiscard]] int columnCount() const;
  [[nodiscard]] QVariant data(int column) const;
  [[nodiscard]] int row() const;

  // parent
  BundleStationItem* parentItem();
  void setParentItem(BundleStationItem* parent);

  // #################
  // getter and setter
  // #################

  // station id
  [[nodiscard]] const int& getStationId() const;

  // key
  [[nodiscard]] const QString& getKey() const;

  // value
  [[nodiscard]] bool getValue() const;
  void setValue(const bool& value);

 private:
  // ##########
  // attributes
  // ##########

  // display values
  QString key;
  int stationId;
  bool value;

  // children and parent
  QList<BundleStationItem*> m_childItems;
  BundleStationItem* m_parentItem;
};

#endif  // BUNDLESTATIONITEM_H

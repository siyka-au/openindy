#ifndef OI_APP_MODELS_TREEITEMS_BUNDLE_STATION_ITEM_H_
#define OI_APP_MODELS_TREEITEMS_BUNDLE_STATION_ITEM_H_

#include <QVariant>
#include <vector>

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
  std::vector<BundleStationItem*> m_childItems;
  BundleStationItem* m_parentItem;
};

#endif  // OI_APP_MODELS_TREEITEMS_BUNDLE_STATION_ITEM_H_

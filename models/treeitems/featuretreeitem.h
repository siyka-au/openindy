#ifndef FEATURETREEITEM_H
#define FEATURETREEITEM_H

#include <QList>
#include <QObject>
#include <QPointer>

#include "featurewrapper.h"
#include "observation.h"
#include "reading.h"
#include "types.h"
#include "util.h"

using namespace oi;

/*!
 * \brief The FeatureTreeItem class
 * Class used to hold single items of the FeatureTreeViewModel
 */
class FeatureTreeItem : public QObject {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(FeatureTreeItem)

 public:
  explicit FeatureTreeItem(const QVariant& display, QObject* parent = nullptr);
  explicit FeatureTreeItem(const QPointer<FeatureWrapper>& feature,
                           QObject* parent = nullptr);
  explicit FeatureTreeItem(const QPointer<Observation>& observation,
                           QObject* parent = nullptr);
  explicit FeatureTreeItem(const QPointer<Reading>& reading,
                           QObject* parent = nullptr);
  ~FeatureTreeItem() override;

  // ##########################################
  // get or set information about the tree item
  // ##########################################

  // query what kind of tree item this item is
  [[nodiscard]] const bool& getIsHeader() const;
  [[nodiscard]] const bool& getIsFeature() const;
  [[nodiscard]] const bool& getIsObservation() const;
  [[nodiscard]] const bool& getIsReading() const;

  // get pointer to corresponding element
  [[nodiscard]] const QPointer<FeatureWrapper>& getFeature() const;
  [[nodiscard]] const QPointer<Observation>& getObservation() const;
  [[nodiscard]] const QPointer<Reading>& getReading() const;

  int getId();

  // get the type of the corresponding element
  [[nodiscard]] ElementTypes getElementType() const;

  // get information about child items
  [[nodiscard]] bool getHasElement(const ElementTypes& typeOfElement) const;

  // get or set children of this tree item
  [[nodiscard]] int getChildCount() const;
  void appendChild(const QPointer<FeatureTreeItem>& child);
  QPointer<FeatureTreeItem> getChild(const int& index);
  // void removeChild(const int &index);
  void deleteChildren();

  // get the index of this tree item in the children list of its parent
  [[nodiscard]] const int& getIndex() const;

  // get parent of this tree item
  [[nodiscard]] bool hasParent() const;
  [[nodiscard]] const QPointer<FeatureTreeItem>& getParent() const;

  // get the value to display in the tree view
  [[nodiscard]] const QVariant& getDisplayValue() const;

 private:
  // #############################################
  // set what kind of tree item this item shall be
  // #############################################

  void setIsHeader();
  void setIsFeature();
  void setIsObservation();
  void setIsReading();

  // #################################
  // set information about child items
  // #################################

  void setHasElement(const bool& hasElement, const ElementTypes& typeOfElement);

  // ################################################
  // set up children depending on the type of element
  // ################################################

  void setUpFeature();
  void setUpObservation();
  void setUpReading();

  // ####################################################
  // attributes that hold information about the tree item
  // ####################################################

  // define what kind of tree item this item is
  bool isHeader{};
  bool isFeature{};
  bool isObservation{};
  bool isReading{};

  // pointer to the corresponding element
  QPointer<FeatureWrapper> feature;
  QPointer<Observation> observation;
  QPointer<Reading> reading;

  // all elements that are sub elements of this item
  QList<ElementTypes> containedElementTypes;

  // parent and child items
  QPointer<FeatureTreeItem> parent;
  QList<QPointer<FeatureTreeItem> > children;

  // index of this tree item in the children list of its parent
  int index;

  // number of children of this tree item
  int childCount;

  // value that is displayed in tree view for this item
  QVariant displayValue;
};

#endif  // FEATURETREEITEM_H

#ifndef PLUGINTREEITEM_H
#define PLUGINTREEITEM_H

#include <QList>
#include <QVariant>

#include "oimetadata.h"
#include "pluginrepository.h"
#include "types.h"
#include "util.h"

using namespace oi;

class PluginTreeItem : public QObject {
  Q_OBJECT

 public:
  explicit PluginTreeItem(const QVariant& display, QObject* parent = nullptr);
  explicit PluginTreeItem(const sys::Plugin& plugin, QObject* parent = nullptr);
  explicit PluginTreeItem(const sys::Function& function,
                          QObject* parent = nullptr);
  explicit PluginTreeItem(const sys::Sensor& sensor, QObject* parent = nullptr);
  explicit PluginTreeItem(const sys::Tool& tool, QObject* parent = nullptr);
  explicit PluginTreeItem(const sys::Simulation& simulation,
                          QObject* parent = nullptr);
  explicit PluginTreeItem(const sys::Exchange& exchange,
                          QObject* parent = nullptr);
  ~PluginTreeItem() override;

  // ##########################################
  // get or set information about the tree item
  // ##########################################

  // get the value to display in the tree view
  const QVariant& getDisplayValue() const;

  // get or set children of this tree item
  int getChildCount() const;
  void appendChild(const QPointer<PluginTreeItem>& child);
  QPointer<PluginTreeItem> getChild(const int& index);
  // void removeChild(const int &index);
  void deleteChildren();

  // get parent of this tree item
  bool hasParent() const;
  const QPointer<PluginTreeItem>& getParent() const;

  // get the index of this tree item in the children list of its parent
  const int& getIndex() const;

  // query what kind of tree item this item is
  const bool& getIsHeader() const;
  const bool& getIsPlugin() const;
  const bool& getIsFunction() const;
  const bool& getIsSensor() const;
  const bool& getIsTool() const;
  const bool& getIsSimulation() const;
  const bool& getIsExchange() const;

  // get object behind tree item
  const sys::Plugin& getPlugin() const;
  const sys::Function& getFunction() const;
  const sys::Sensor& getSensor() const;
  const sys::Tool& getTool() const;
  const sys::Simulation& getSimulation() const;
  const sys::Exchange& getExchange() const;

 private:
  // #############################################
  // set what kind of tree item this item shall be
  // #############################################

  void setIsHeader();
  void setIsPlugin();
  void setIsFunction();
  void setIsSensor();
  void setIsTool();
  void setIsSimulation();
  void setIsExchange();

  // ###########################################
  // set up children depending on a given plugin
  // ###########################################

  void setUpPlugin();

  // ####################################################
  // attributes that hold information about the tree item
  // ####################################################

  // value that is displayed in tree view for this item
  QVariant displayValue;

  // number of children of this tree item
  int childCount;

  // index of this tree item in the children list of its parent
  int index;

  // parent and child items
  QPointer<PluginTreeItem> parent;
  QList<QPointer<PluginTreeItem> > children;

  // define what kind of tree item this item is
  bool isHeader{};
  bool isPlugin{};
  bool isFunction{};
  bool isSensor{};
  bool isTool{};
  bool isSimulation{};
  bool isExchange{};

  // object behind the tree item
  sys::Plugin plugin;
  sys::Function function;
  sys::Sensor sensor;
  sys::Tool tool;
  sys::Simulation simulation;
  sys::Exchange exchange;
};

#endif  // PLUGINTREEITEM_H

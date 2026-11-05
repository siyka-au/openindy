#ifndef PLUGINTREEVIEWMODEL_H
#define PLUGINTREEVIEWMODEL_H

#include <QAbstractItemModel>
#include <QModelIndex>
#include <QPixmap>

#include "pluginrepository.h"
#include "plugintreeitem.h"

using namespace oi;

/*!
 * \brief The PluginTreeViewModel class
 * Model that holds all plugins currently loaded into OpenIndy
 */
class PluginTreeViewModel : public QAbstractItemModel {
  Q_OBJECT

 public:
  explicit PluginTreeViewModel(QObject* parent = nullptr);

  // #######################################
  // override methods of abstract item model
  // #######################################

  QModelIndex index(int row, int column,
                    const QModelIndex& parent = QModelIndex()) const override;
  QModelIndex parent(const QModelIndex& index) const override;

  int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  int columnCount(const QModelIndex& parent = QModelIndex()) const override;

  QVariant data(const QModelIndex& index,
                int role = Qt::DisplayRole) const override;

  QVariant headerData(int section, Qt::Orientation orientation,
                      int role = Qt::DisplayRole) const override;

  // ##################
  // get or set plugins
  // ##################

  const QList<sys::Plugin>& getPlugins() const;
  void setPlugins(const QList<sys::Plugin>& plugins);

 signals:

  // #########################
  // send messages to OpenIndy
  // #########################

  void sendMessage(const QString& msg, const MessageTypes& msgType,
                   const MessageDestinations& msgDest = eConsoleMessage);

 private:
  // ##############
  // loaded plugins
  // ##############

  QList<sys::Plugin> plugins;

  // ##############
  // root tree item
  // ##############

  PluginTreeItem* rootItem;
};

#endif  // PLUGINTREEVIEWMODEL_H

#ifndef OI_APP_MODELS_PLUGIN_TREE_VIEW_MODEL_H_
#define OI_APP_MODELS_PLUGIN_TREE_VIEW_MODEL_H_

#include <QAbstractItemModel>
#include <QModelIndex>
#include <QPixmap>
#include <vector>

#include "plugin_repository.h"
#include "plugin_tree_item.h"

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

  [[nodiscard]] QModelIndex index(
      int row, int column,
      const QModelIndex& parent = QModelIndex()) const override;
  [[nodiscard]] QModelIndex parent(const QModelIndex& index) const override;

  [[nodiscard]] int rowCount(
      const QModelIndex& parent = QModelIndex()) const override;
  [[nodiscard]] int columnCount(
      const QModelIndex& parent = QModelIndex()) const override;

  [[nodiscard]] QVariant data(const QModelIndex& index,
                              int role = Qt::DisplayRole) const override;

  [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation,
                                    int role = Qt::DisplayRole) const override;

  // ##################
  // get or set plugins
  // ##################

  [[nodiscard]] const std::vector<sys::Plugin>& getPlugins() const;
  void setPlugins(const std::vector<sys::Plugin>& plugins);

 signals:

  // #########################
  // send messages to OpenIndy
  // #########################

  void sendMessage(const QString& msg, const MessageTypes& msgType,
                   const MessageDestinations& msgDest =
                       MessageDestinations::eConsoleMessage);

 private:
  // ##############
  // loaded plugins
  // ##############

  std::vector<sys::Plugin> plugins;

  // ##############
  // root tree item
  // ##############

  PluginTreeItem* rootItem;
};

#endif  // OI_APP_MODELS_PLUGIN_TREE_VIEW_MODEL_H_

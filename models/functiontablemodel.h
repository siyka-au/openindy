#ifndef FUNCTIONTABLEMODEL_H
#define FUNCTIONTABLEMODEL_H

#include <QAbstractTableModel>
#include <QList>
#include <QObject>
#include <QPointer>
#include <vector>

#include "oijob.h"
#include "pluginrepository.h"
#include "types.h"
#include "util.h"

using namespace oi;

/*!
 * \brief The FunctionTableModel class
 * Model that holds all available function plugins
 */
class FunctionTableModel : public QAbstractTableModel {
  Q_OBJECT

 public:
  explicit FunctionTableModel(QObject* parent = nullptr);

  // ########################################
  // override methods of abstract table model
  // ########################################

  [[nodiscard]] int rowCount(
      const QModelIndex& parent = QModelIndex()) const override;
  [[nodiscard]] int columnCount(
      const QModelIndex& parent = QModelIndex()) const override;

  [[nodiscard]] QVariant data(const QModelIndex& index,
                              int role = Qt::DisplayRole) const override;

  [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation,
                                    int role = Qt::DisplayRole) const override;

  // ########################
  // get function information
  // ########################

  [[nodiscard]] QString getFunctionDescription(const QModelIndex& index) const;
  [[nodiscard]] QString getFunctionName(const QModelIndex& index) const;
  [[nodiscard]] QString getPluginFilePath(const QModelIndex& index) const;

  // ######################################
  // get or set currently available plugins
  // ######################################

  [[nodiscard]] const std::vector<sys::Plugin>& getPlugins() const;
  void setPlugins(const std::vector<sys::Plugin>& plugins);

 signals:

  // #########################
  // send messages to OpenIndy
  // #########################

  void sendMessage(const QString& msg, const MessageTypes& msgType,
                   const MessageDestinations& msgDest =
                       MessageDestinations::eConsoleMessage);

 private slots:

  // ##############################################
  // update the model when the plugins were changed
  // ##############################################

  void updateModel();

 private:
  // ###########################
  // currently available plugins
  // ###########################

  std::vector<sys::Plugin> plugins;
};

#endif  // FUNCTIONTABLEMODEL_H

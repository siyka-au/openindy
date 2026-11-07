#ifndef AVAILABLEFUNCTIONSLISTMODEL_H
#define AVAILABLEFUNCTIONSLISTMODEL_H

#include <QAbstractListModel>
#include <QList>
#include <QModelIndex>
#include <QPair>
#include <QPixmap>
#include <QPointer>
#include <vector>

#include "function.h"
#include "pluginrepository.h"
#include "settings.h"
#include "types.h"
#include "util.h"

using namespace oi;

/*!
 * \brief The AvailableFunctionsListModel class
 * Model that holds all available create functions (possible first functions of
 * a feature)
 */
class AvailableFunctionsListModel : public QAbstractListModel {
  Q_OBJECT

 public:
  explicit AvailableFunctionsListModel(QObject* parent = nullptr);

  // #######################################
  // override methods of abstract item model
  // #######################################

  [[nodiscard]] int rowCount(
      const QModelIndex& parent = QModelIndex()) const override;
  [[nodiscard]] int columnCount(
      const QModelIndex& parent = QModelIndex()) const override;

  [[nodiscard]] QVariant data(const QModelIndex& index,
                              int role = Qt::DisplayRole) const override;

  [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation,
                                    int role = Qt::DisplayRole) const override;

  // #########################
  // get the function at index
  // #########################

  [[nodiscard]] sys::Function getFunctionAtIndex(
      const QModelIndex& index) const;

  // ############################
  // get or set default functions
  // ############################

  [[nodiscard]] QPair<QString, QString> getDefaultFunction(
      const FeatureTypes& type) const;

  // ############################
  // get or set available plugins
  // ############################

  [[nodiscard]] const std::vector<sys::Plugin>& getPlugins() const;
  void setPlugins(const std::vector<sys::Plugin>& plugins);

 signals:

  // ###########################
  // signals to change job state
  // ###########################

  void startRemoveUsedElements(const oi::Guarded<FeatureWrapper>& target,
                               const int& functionPosition,
                               const int& neededElementsIndex,
                               const int& elementId);

  // #########################
  // send messages to OpenIndy
  // #########################

  void sendMessage(const QString& msg, const MessageTypes& msgType,
                   const MessageDestinations& msgDest =
                       MessageDestinations::eConsoleMessage);

 private slots:

  // ###########################################
  // update the model when job state has changed
  // ###########################################

  void updateModel();

 private:
  // #################
  // available plugins
  // #################

  std::vector<sys::Plugin> plugins;
};

#endif  // AVAILABLEFUNCTIONSLISTMODEL_H

#ifndef SENSORCONFIGURATIONPROXYMODEL_H
#define SENSORCONFIGURATIONPROXYMODEL_H

#include <QPointer>
#include <QSortFilterProxyModel>

#include "sensorconfigurationmodel.h"
#include "types.h"
#include "util.h"

using namespace oi;

/*!
 * \brief The SensorConfigurationProxyModel class
 * Model that holds all available sensor configurations filtered by saved and
 * project property
 */
class SensorConfigurationProxyModel : public QSortFilterProxyModel {
  Q_OBJECT

 public:
  explicit SensorConfigurationProxyModel(QObject* parent = nullptr);

  // ##########
  // set filter
  // ##########

  void setFilter(const bool& showAll);

  // ###########################################
  // override methods of sort filter proxy model
  // ###########################################

  [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation,
                                    int role) const override;

 signals:

  // #########################
  // send messages to OpenIndy
  // #########################

  void sendMessage(const QString& msg, const MessageTypes& msgType,
                   const MessageDestinations& msgDest =
                       MessageDestinations::eConsoleMessage);

 protected:
  [[nodiscard]] bool filterAcceptsRow(
      int source_row, const QModelIndex& source_parent) const override;
  [[nodiscard]] bool filterAcceptsColumn(
      int source_column, const QModelIndex& source_parent) const override;

 private:
  // ################
  // filter attribute
  // ################

  bool showAll;
};

#endif  // SENSORCONFIGURATIONPROXYMODEL_H

#ifndef BUNDLEPARAMETERTABLEPROXYMODEL_H
#define BUNDLEPARAMETERTABLEPROXYMODEL_H

#include <QPointer>
#include <QSortFilterProxyModel>

#include "feature_table_model.h"
#include "oi_job.h"
#include "trafo_param_table_column_config.h"

using namespace oi;

/*!
 * \brief The BundleParameterTableProxyModel class
 */
class BundleParameterTableProxyModel : public QSortFilterProxyModel {
  Q_OBJECT

 public:
  explicit BundleParameterTableProxyModel(QObject* parent = nullptr);

  // ###########################################
  // override methods of sort filter proxy model
  // ###########################################

  [[nodiscard]] QModelIndex mapFromSource(
      const QModelIndex& sourceIndex) const override;
  [[nodiscard]] QModelIndex mapToSource(
      const QModelIndex& proxyIndex) const override;

 signals:

  // #########################
  // send messages to OpenIndy
  // #########################

  void sendMessage(const QString& msg, const MessageTypes& msgType,
                   const MessageDestinations& msgDest =
                       MessageDestinations::eConsoleMessage) const;

 protected:
  // ###########################################
  // override methods of sort filter proxy model
  // ###########################################

  [[nodiscard]] bool filterAcceptsRow(
      int source_row, const QModelIndex& source_parent) const override;
  [[nodiscard]] bool filterAcceptsColumn(
      int source_column, const QModelIndex& source_parent) const override;

 private:
  // ##############
  // display config
  // ##############

  TrafoParamTableColumnConfig trafoParamTableColumnConfig;

  // #############
  // helper method
  // #############

  [[nodiscard]] bool isBundleStationTrafo(
      const oi::Guarded<TrafoParam>& param) const;
};

#endif  // BUNDLEPARAMETERTABLEPROXYMODEL_H

#ifndef BUNDLEPARAMETERTABLEPROXYMODEL_H
#define BUNDLEPARAMETERTABLEPROXYMODEL_H

#include <QPointer>
#include <QSortFilterProxyModel>

#include "featuretablemodel.h"
#include "oijob.h"
#include "trafoparamtablecolumnconfig.h"

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

  QModelIndex mapFromSource(const QModelIndex& sourceIndex) const override;
  QModelIndex mapToSource(const QModelIndex& proxyIndex) const override;

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

  bool filterAcceptsRow(int source_row,
                        const QModelIndex& source_parent) const override;
  bool filterAcceptsColumn(int source_column,
                           const QModelIndex& source_parent) const override;

 private:
  // ##############
  // display config
  // ##############

  TrafoParamTableColumnConfig trafoParamTableColumnConfig;

  // #############
  // helper method
  // #############

  bool isBundleStationTrafo(const QPointer<TrafoParam>& param) const;
};

#endif  // BUNDLEPARAMETERTABLEPROXYMODEL_H

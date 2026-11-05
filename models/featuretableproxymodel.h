#ifndef FEATURETABLEPROXYMODEL_H
#define FEATURETABLEPROXYMODEL_H

#include <QPointer>
#include <QSortFilterProxyModel>

#include "featuresorter.h"
#include "featuretablecolumnconfig.h"
#include "featuretablemodel.h"
#include "oijob.h"

using namespace oi;

/*!
 * \brief The FeatureTableProxyModel class
 */
class FeatureTableProxyModel : public QSortFilterProxyModel {
  Q_OBJECT

 public:
  explicit FeatureTableProxyModel(QObject* parent = nullptr);

  // ###############################
  // get or set current OpenIndy job
  // ###############################

  [[nodiscard]] const QPointer<OiJob>& getCurrentJob() const;
  void setCurrentJob(const QPointer<OiJob>& job);

  // #########################
  // get or set display config
  // #########################

  [[nodiscard]] const FeatureTableColumnConfig& getFeatureTableColumnConfig()
      const;
  void setFeatureTableColumnConfig(const FeatureTableColumnConfig& config);

  // ###########################################
  // override methods of sort filter proxy model
  // ###########################################

  [[nodiscard]] QModelIndex mapFromSource(
      const QModelIndex& sourceIndex) const override;
  [[nodiscard]] QModelIndex mapToSource(
      const QModelIndex& proxyIndex) const override;

  void setSortingMode(FeatureSorter::SortingMode mode);

 signals:

  // #########################
  // send messages to OpenIndy
  // #########################

  void sendMessage(const QString& msg, const MessageTypes& msgType,
                   const MessageDestinations& msgDest =
                       MessageDestinations::eConsoleMessage) const;

 protected:
  [[nodiscard]] bool filterAcceptsRow(
      int source_row, const QModelIndex& source_parent) const override;
  [[nodiscard]] bool filterAcceptsColumn(
      int source_column, const QModelIndex& source_parent) const override;

  [[nodiscard]] bool lessThan(const QModelIndex& left,
                              const QModelIndex& right) const override;

 private:
  // ##############
  // display config
  // ##############

  FeatureTableColumnConfig featureTableColumnConfig;

  // ###########
  // current job
  // ###########

  QPointer<OiJob> job;

  // ##############
  // feature sorter
  // ##############

  FeatureSorter sorter;
};

#endif  // FEATURETABLEPROXYMODEL_H

#ifndef OI_APP_MODELS_FEATURE_TABLE_PROXY_MODEL_H_
#define OI_APP_MODELS_FEATURE_TABLE_PROXY_MODEL_H_

#include <QPointer>
#include <QSortFilterProxyModel>

#include "feature_sorter.h"
#include "feature_table_column_config.h"
#include "feature_table_model.h"
#include "oi_job.h"

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

  [[nodiscard]] const oi::Guarded<OiJob>& getCurrentJob() const;
  void setCurrentJob(const oi::Guarded<OiJob>& job);

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

  oi::Guarded<OiJob> job;

  // ##############
  // feature sorter
  // ##############

  FeatureSorter sorter;
};

#endif  // OI_APP_MODELS_FEATURE_TABLE_PROXY_MODEL_H_

#ifndef FEATUREDIFFERENCEPROXYMODEL_H
#define FEATUREDIFFERENCEPROXYMODEL_H

#include <QPointer>
#include <QSortFilterProxyModel>

#include "featuredifferencetablemodel.h"
#include "featuresorter.h"
#include "oijob.h"

using namespace oi;

/*!
 * \brief The FeatureDifferenceProxyModel class
 */
class FeatureDifferenceProxyModel : public QSortFilterProxyModel {
  Q_OBJECT

 public:
  explicit FeatureDifferenceProxyModel(QObject* parent = nullptr);

  // ###############################
  // get or set current OpenIndy job
  // ###############################

  [[nodiscard]] const oi::Guarded<OiJob>& getCurrentJob() const;
  void setCurrentJob(const oi::Guarded<OiJob>& job);

  void setSortingMode(FeatureSorter::SortingMode mode);

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

  [[nodiscard]] bool lessThan(const QModelIndex& left,
                              const QModelIndex& right) const override;

 private:
  // ###########
  // current job
  // ###########

  oi::Guarded<OiJob> job;

  FeatureSorter sorter;
};

#endif  // FEATUREDIFFERENCEPROXYMODEL_H

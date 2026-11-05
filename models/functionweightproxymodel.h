#ifndef FUNCTIONWEIGHTPROXYMODEL_H
#define FUNCTIONWEIGHTPROXYMODEL_H

#include <QPointer>
#include <QSortFilterProxyModel>

#include "functionweightstablemodel.h"
#include "oijob.h"

using namespace oi;

/*!
 * \brief The FunctionWeightProxyModel class
 */
class FunctionWeightProxyModel : public QSortFilterProxyModel {
  Q_OBJECT

 public:
  explicit FunctionWeightProxyModel(QObject* parent = nullptr);

  // ###############################
  // get or set current OpenIndy job
  // ###############################

  [[nodiscard]] const QPointer<OiJob>& getCurrentJob() const;
  void setCurrentJob(const QPointer<OiJob>& job);

  // ###########################################
  // override methods of sort filter proxy model
  // ###########################################

  [[nodiscard]] QModelIndex mapFromSource(
      const QModelIndex& sourceIndex) const override;
  [[nodiscard]] QModelIndex mapToSource(
      const QModelIndex& proxyIndex) const override;

 protected:
  [[nodiscard]] bool filterAcceptsRow(
      int source_row, const QModelIndex& source_parent) const override;
  [[nodiscard]] bool filterAcceptsColumn(
      int source_column, const QModelIndex& source_parent) const override;
  [[nodiscard]] bool lessThan(const QModelIndex& source_left,
                              const QModelIndex& source_right) const override;

 private:
  QPointer<OiJob> job;
};

#endif  // FUNCTIONWEIGHTPROXYMODEL_H

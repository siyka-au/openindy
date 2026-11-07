#ifndef FUNCTIONTABLEPROXYMODEL_H
#define FUNCTIONTABLEPROXYMODEL_H

#include <QPointer>
#include <QSortFilterProxyModel>

#include "functiontablemodel.h"
#include "oijob.h"
#include "oimetadata.h"
#include "pluginmanager.h"
#include "types.h"
#include "util.h"

using namespace oi;

/*!
 * \brief The FunctionTableProxyModel class
 * Model that holds all available function plugins filtered by the function
 * types that could be added to the active feature's function list
 */
class FunctionTableProxyModel : public QSortFilterProxyModel {
  Q_OBJECT

 public:
  explicit FunctionTableProxyModel(QObject* parent = nullptr);

  // ####################################
  // add a function to the active feature
  // ####################################

  void addFunction(const QModelIndex& index);

  // ###############################
  // get or set current OpenIndy job
  // ###############################

  [[nodiscard]] const oi::Guarded<OiJob>& getCurrentJob() const;
  void setCurrentJob(const oi::Guarded<OiJob>& job);

  [[nodiscard]] QVariant data(const QModelIndex& index,
                              int role) const override;

 signals:

  // ###########################
  // signals to change job state
  // ###########################

  // #########################
  // send messages to OpenIndy
  // #########################

  void sendMessage(const QString& msg, const MessageTypes& msgType,
                   const MessageDestinations& msgDest =
                       MessageDestinations::eConsoleMessage);

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
  // helper methods
  // ##############

  void connectJob();
  void disconnectJob();

  // ###########
  // current job
  // ###########

  oi::Guarded<OiJob> currentJob;
};

#endif  // FUNCTIONTABLEPROXYMODEL_H

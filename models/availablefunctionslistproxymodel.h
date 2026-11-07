#ifndef AVAILABLEFUNCTIONSLISTPROXYMODEL_H
#define AVAILABLEFUNCTIONSLISTPROXYMODEL_H

#include <QPointer>
#include <QSortFilterProxyModel>

#include "availablefunctionslistmodel.h"
#include "oijob.h"
#include "oimetadata.h"
#include "pluginmanager.h"
#include "types.h"
#include "util.h"

#if defined(OI_MAIN_LIB)
#define OI_MAIN_EXPORT Q_DECL_EXPORT
#else
#define OI_MAIN_EXPORT Q_DECL_IMPORT
#endif

using namespace oi;

/*!
 * \brief The AvailableFunctionsListProxyModel class
 * Model that holds all available functions filtered by feature type
 */
class OI_MAIN_EXPORT AvailableFunctionsListProxyModel
    : public QSortFilterProxyModel {
  Q_OBJECT

 public:
  explicit AvailableFunctionsListProxyModel(QObject* parent = nullptr);

  // ######################
  // filter by feature type
  // ######################

  void setFilter(const FeatureTypes& type);

  // #########################
  // get the function at index
  // #########################

  [[nodiscard]] sys::Function getFunctionAtIndex(const int& index) const;

 signals:

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
  // #################
  // helper attributes
  // #################

  FeatureTypes filter;
};

#endif  // AVAILABLEFUNCTIONSLISTPROXYMODEL_H

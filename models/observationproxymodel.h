#ifndef OBSERVATIONPROXYMODEL_H
#define OBSERVATIONPROXYMODEL_H

#include <QPointer>
#include <QSortFilterProxyModel>

#include "observationmodel.h"
#include "observationtablecolumnconfig.h"
#include "oijob.h"

using namespace oi;

/*!
 * \brief The ObservationProxyModel class
 */
class ObservationProxyModel : public QSortFilterProxyModel {
  Q_OBJECT

 public:
  explicit ObservationProxyModel(QObject* parent = nullptr);

  // #########################
  // get or set display config
  // #########################

  const ObservationTableColumnConfig& getObservationTableColumnConfig() const;
  void setObservationTableColumnConfig(
      const ObservationTableColumnConfig& config);

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

  bool filterAcceptsRow(int source_row,
                        const QModelIndex& source_parent) const override;
  bool filterAcceptsColumn(int source_column,
                           const QModelIndex& source_parent) const override;
  bool lessThan(const QModelIndex& source_left,
                const QModelIndex& source_right) const override;

 private:
  // ##############
  // display config
  // ##############

  ObservationTableColumnConfig observationTableColumnConfig;
};

#endif  // OBSERVATIONPROXYMODEL_H

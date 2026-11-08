#ifndef READINGPROXYMODEL_H
#define READINGPROXYMODEL_H

#include <QPointer>
#include <QSortFilterProxyModel>

#include "oi_job.h"
#include "reading_model.h"
#include "reading_table_column_config.h"

using namespace oi;

/*!
 * \brief The ReadingProxyModel class
 */
class ReadingProxyModel : public QSortFilterProxyModel {
  Q_OBJECT

 public:
  explicit ReadingProxyModel(QObject* parent = nullptr);

  // #########################
  // get or set display config
  // #########################

  [[nodiscard]] const ReadingTableColumnConfig& getReadingTableColumnConfig()
      const;
  void setReadingTableColumnConfig(const ReadingTableColumnConfig& config);

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
  [[nodiscard]] bool lessThan(const QModelIndex& source_left,
                              const QModelIndex& source_right) const override;

 private:
  // ##############
  // display config
  // ##############

  ReadingTableColumnConfig readingTableColumnConfig;
};

#endif  // READINGPROXYMODEL_H

#ifndef OI_APP_CONTROLLER_CLIPBOARD_UTIL_H_
#define OI_APP_CONTROLLER_CLIPBOARD_UTIL_H_

#include <QApplication>
#include <QClipboard>
#include <QDebug>
#include <QItemSelectionModel>
#include <QModelIndex>
#include <QModelIndexList>
#include <QPointer>
#include <QString>

#include "types.h"

using namespace oi;

class ClipBoardUtil : public QObject {
  Q_OBJECT
 public:
  // ClipBoardUtil();

  // copy displayed values
  void copySelectionAsCsvToClipBoard(
      QPointer<QAbstractItemModel> model,
      QPointer<QItemSelectionModel> selectionModel);
  // copy text to clipboard
  void copyToClipBoard(QString coyp_table);

 signals:
  void sendMessage(const QString& msg, const MessageTypes& msgType,
                   const MessageDestinations& msgDest =
                       MessageDestinations::eConsoleMessage) const;
};

#endif  // OI_APP_CONTROLLER_CLIPBOARD_UTIL_H_

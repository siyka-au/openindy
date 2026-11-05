#ifndef OIWEBSOCKETSERVER_H
#define OIWEBSOCKETSERVER_H

#include <QList>
#include <QPointer>
#include <QWebSocketServer>

#include "oirequesthandler.h"
#include "oiwebsocket.h"
#include "oiwebsocketstreamer.h"
#include "types.h"

/*!
 * \brief The OiWebSocketServer class
 * Class to handle all web socket connections to OpenIndy
 */
class OiWebSocketServer : public QWebSocketServer {
  Q_OBJECT

 public:
  explicit OiWebSocketServer(QObject* parent = 0);
  ~OiWebSocketServer();

  static int generateUniqueId();

 signals:

  // #########################
  // send messages to OpenIndy
  // #########################

  void sendMessage(const QString& msg, const MessageTypes& msgType,
                   const MessageDestinations& msgDest = eConsoleMessage);

  // ##########################
  // send a request to OpenIndy
  // ##########################

  void sendRequest(oi::OiRequestResponse request);

  // ##############################
  // send response to the requester
  // ##############################

  void receiveResponse(const OiRequestResponse& response);

 public slots:

  // #################
  // server connection
  // #################

  void startServer();
  void stopServer();

 private slots:

  // ######################
  // new client connections
  // ######################

  void incomingConnection();

 private:
  // #################
  // helper attributes
  // #################

  QPointer<OiWebSocketStreamer> streamer;

  static int currentId;
  QList<QPointer<OiWebSocket> > usedSockets;
};

#endif  // OIWEBSOCKETSERVER_H

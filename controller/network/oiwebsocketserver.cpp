#include "oiwebsocketserver.h"

int OiWebSocketServer::currentId = 0;

/*!
 * \brief OiWebSocketServer::OiWebSocketServer
 * \param parent
 */
OiWebSocketServer::OiWebSocketServer(QObject* parent)
    : QWebSocketServer("OiServer", QWebSocketServer::NonSecureMode, parent),
      streamer(new OiWebSocketStreamer()) {
  // create web socket streamer

  // listen to incoming connections
  QObject::connect(this, &OiWebSocketServer::newConnection, this,
                   &OiWebSocketServer::incomingConnection, Qt::AutoConnection);
}

/*!
 * \brief OiWebSocketServer::~OiWebSocketServer
 */
OiWebSocketServer::~OiWebSocketServer() { this->stopServer(); }

/*!
 * \brief OiWebSocketServer::generateUniqueId
 * Generates unique id's for OiWebsocket's
 * \return
 */
int OiWebSocketServer::generateUniqueId() {
  OiWebSocketServer::currentId++;
  return OiWebSocketServer::currentId;
}

/*!
 * \brief OiServer::startServer
 * Starts the server so that clients are able to connect to OpenIndy
 */
void OiWebSocketServer::startServer() {
  if (!this->listen(QHostAddress::Any, 1235)) {
    emit this->sendMessage("Cannot start local server",
                           MessageTypes::eErrorMessage);
  } else {
    emit this->sendMessage("Local server ready ...",
                           MessageTypes::eInformationMessage);
  }
}

/*!
 * \brief OiServer::stopServer
 * Stops the server so that clients are not able to connect to OpenIndy anymore
 */
void OiWebSocketServer::stopServer() {
  // stop listening for new connections
  if (this->isListening()) {
    this->close();
  }

  // close the already existing sockets
  for (const auto& usedSocket : this->usedSockets) {
    if (!usedSocket.isNull()) {
      usedSocket->close();
      delete usedSocket;
    }
  }
}

/*!
 * \brief OiServer::incomingConnection
 * Is called whenever a client tries to connect to OpenIndy
 * \param socketDescriptor
 */
void OiWebSocketServer::incomingConnection() {
  // create web socket instance
  QPointer<QWebSocket> mySocket = this->nextPendingConnection();
  QPointer<OiWebSocket> myConnection = new OiWebSocket();
  myConnection->setSocket(mySocket);

  emit this->sendMessage(
      QString("Connecting to client %1 ...")
          .arg(QString::number(myConnection->getInternalRef())),
      MessageTypes::eInformationMessage);

  // connect web socket
  QObject::connect(myConnection, &OiWebSocket::disconnected, myConnection,
                   &OiWebSocket::deleteLater, Qt::AutoConnection);
  QObject::connect(myConnection, &OiWebSocket::sendRequest, this,
                   &OiWebSocketServer::sendRequest, Qt::AutoConnection);
  QObject::connect(this, &OiWebSocketServer::receiveResponse, myConnection,
                   &OiWebSocket::receiveResponse, Qt::AutoConnection);

  // add web socket to list of used sockets
  this->usedSockets.append(myConnection);
}

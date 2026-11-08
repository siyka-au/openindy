#include "oi_web_socket.h"

#include "oi_web_socket_server.h"
#include "qt_convert.h"

/*!
 * \brief OiWebSocket::OiWebSocket
 * \param parent
 */
OiWebSocket::OiWebSocket(QObject* parent)
    : internalRef(OiWebSocketServer::generateUniqueId()), QObject(parent) {}

/*!
 * \brief OiWebSocket::~OiWebSocket
 */
OiWebSocket::~OiWebSocket() {
  // delete web socket
  if (!this->socket.isNull()) {
    delete this->socket;
  }
}

/*!
 * \brief OiNetworkConnection::setSocket
 * \param socketDescriptor
 * \return
 */
bool OiWebSocket::setSocket(const QPointer<QWebSocket>& mySocket) {
  // check and set socket
  if (!this->socket.isNull() || mySocket.isNull()) {
    return false;
  }
  this->socket = mySocket;

  // connect socket
  QObject::connect(this->socket, &QWebSocket::binaryMessageReceived, this,
                   &OiWebSocket::readMessage, Qt::AutoConnection);
  QObject::connect(this->socket, &QWebSocket::connected, this,
                   &OiWebSocket::connected, Qt::AutoConnection);
  QObject::connect(this->socket, &QWebSocket::disconnected, this,
                   &OiWebSocket::disconnected, Qt::AutoConnection);

  return true;
}

/*!
 * \brief OiNetworkConnection::getInternalRef
 * \return
 */
int OiWebSocket::getInternalRef() const { return this->internalRef; }

/*!
 * \brief OiWebSocket::close
 */
void OiWebSocket::close() {
  if (!this->socket.isNull()) {
    this->socket->close();
  }
}

/*!
 * \brief OiNetworkConnection::receiveResponse
 * Is called whenever a response to a request of this client is available
 * \param response
 */
void OiWebSocket::receiveResponse(const oi::OiRequestResponse& response) {
  // send response to client
  if (this->socket.isNull()) {
    return;
  }
  if (this->internalRef == response.requesterId || response.requesterId == -1) {
    this->socket->sendBinaryMessage(
        QByteArray(response.response.data(),
                   static_cast<qsizetype>(response.response.size())));
  }
}

/*!
 * \brief OiNetworkConnection::readMessage
 * Is called whenever a client has sent a message to OpenIndy
 */
void OiWebSocket::readMessage(const QByteArray& msg) {
  // pass the serialized request on; the request handler decodes it
  OiRequestResponse request;
  request.requesterId = this->internalRef;
  request.request = toStdBytes(msg);

  emit this->sendRequest(request);
}

#ifndef OI_APP_CONTROLLER_NETWORK_OI_WEB_SOCKET_H_
#define OI_APP_CONTROLLER_NETWORK_OI_WEB_SOCKET_H_

#include <QObject>
#include <QPointer>
#include <QWebSocket>

#include "oi_request_response.h"
#include "types.h"

/*!
 * \brief The OiWebSocket class
 * Represents a websocket connection of a client to OpenIndy
 */
class OiWebSocket : public QObject {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(OiWebSocket)

 public:
  explicit OiWebSocket(QObject* parent = nullptr);
  ~OiWebSocket() override;

  // ##########
  // set socket
  // ##########

  bool setSocket(const QPointer<QWebSocket>& mySocket);

  // #####################
  // get socket identifier
  // #####################

  [[nodiscard]] int getInternalRef() const;

 signals:

  // ###############################
  // inform about connection changes
  // ###############################

  void connected();
  void disconnected();

  // ##########################
  // send a request to OpenIndy
  // ##########################

  void sendRequest(const oi::OiRequestResponse& request);

 public slots:

  // #######################
  // close socket connection
  // #######################

  void close();

  // ##############################
  // recevie response from OpenIndy
  // ##############################

  void receiveResponse(const oi::OiRequestResponse& response);

 private slots:

  // ##############
  // helper methods
  // ##############

  void readMessage(const QByteArray& msg);

 private:
  // #################
  // helper attributes
  // #################

  QPointer<QWebSocket> socket;
  int internalRef;
};

#endif  // OI_APP_CONTROLLER_NETWORK_OI_WEB_SOCKET_H_

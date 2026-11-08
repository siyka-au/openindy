#ifndef OI_APP_CONTROLLER_NETWORK_OI_WEB_SOCKET_STREAMER_H_
#define OI_APP_CONTROLLER_NETWORK_OI_WEB_SOCKET_STREAMER_H_

#include <QObject>
#include <QPointer>

#include "oi_job.h"
#include "oi_request_response.h"

/*!
 * \brief The OiWebSocketStreamer class
 * Sends information to OpenIndy-Clients asynchronously
 */
class OiWebSocketStreamer : public QObject {
  Q_OBJECT

 public:
  explicit OiWebSocketStreamer(QObject* parent = nullptr);

  // ######################
  // get or set current job
  // ######################

  [[nodiscard]] const oi::Guarded<oi::OiJob>& getJob() const;
  void setJob(const oi::Guarded<oi::OiJob>& job);

 public slots:

 signals:

  // ###########################
  // send information to clients
  // ###########################

 private:
  // #################
  // helper attributes
  // #################

  oi::Guarded<oi::OiJob> currentJob;

  // ##############
  // helper methods
  // ##############

 private slots:
};

#endif  // OI_APP_CONTROLLER_NETWORK_OI_WEB_SOCKET_STREAMER_H_

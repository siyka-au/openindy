#ifndef OIWEBSOCKETSTREAMER_H
#define OIWEBSOCKETSTREAMER_H

#include <QObject>
#include <QPointer>

#include "oijob.h"
#include "oirequestresponse.h"

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

#endif  // OIWEBSOCKETSTREAMER_H

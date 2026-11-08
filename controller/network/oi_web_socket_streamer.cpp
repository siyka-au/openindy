#include "oi_web_socket_streamer.h"

using namespace oi;

/*!
 * \brief OiWebSocketStreamer::OiWebSocketStreamer
 * \param parent
 */
OiWebSocketStreamer::OiWebSocketStreamer(QObject* parent) {}

/*!
 * \brief OiWebSocketStreamer::getJob
 * \return
 */
const oi::Guarded<OiJob>& OiWebSocketStreamer::getJob() const {
  return this->currentJob;
}

/*!
 * \brief OiWebSocketStreamer::setJob
 * \param job
 */
void OiWebSocketStreamer::setJob(const oi::Guarded<OiJob>& job) {
  if (!job.isNull()) {
    this->currentJob = job;
  }
}

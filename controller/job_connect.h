#ifndef JOBCONNECT_H
#define JOBCONNECT_H

#include <QHash>
#include <QList>
#include <QObject>

#include "guarded.h"
#include "oi_job.h"

/*!
 * Connects a Qt signal of `sender` to a member function of an OiJob (which is
 * no QObject any more, so it cannot be the receiver of a plain connect). The
 * call is made in the thread of `sender`, and dropped once the job has been
 * destroyed.
 */
inline QHash<QObject*, QList<QMetaObject::Connection>>& jobConnections() {
  // never destroyed: senders may outlive static destruction
  static auto* connections =
      new QHash<QObject*, QList<QMetaObject::Connection>>();
  return *connections;
}

inline void rememberJobConnection(QObject* sender,
                                  const QMetaObject::Connection& connection) {
  auto& all = jobConnections();
  if (!all.contains(sender)) {
    QObject::connect(sender, &QObject::destroyed, sender,
                     [sender]() { jobConnections().remove(sender); });
  }
  all[sender].append(connection);
}

/*!
 * Severs every connection that connectToJob made for `sender` (what the
 * old per-signal disconnect from the job did, for all of them at once).
 */
inline void disconnectFromJob(QObject* sender) {
  const auto connections = jobConnections().take(sender);
  for (const QMetaObject::Connection& c : connections) QObject::disconnect(c);
}

template <class Sender, class Signal, class Ret, class... Args>
QMetaObject::Connection connectToJob(
    Sender* sender, Signal signal, const oi::Guarded<oi::OiJob>& job,
    Ret (oi::OiJob::*slot)(Args...),
    Qt::ConnectionType type = Qt::AutoConnection) {
  const QMetaObject::Connection c = QObject::connect(
      sender, signal, sender,
      [job, slot](Args... args) {
        if (!job.isNull()) (job.data()->*slot)(args...);
      },
      type);
  rememberJobConnection(sender, c);
  return c;
}

template <class Sender, class Signal, class Ret, class... Args>
QMetaObject::Connection connectToJob(
    Sender* sender, Signal signal, const oi::Guarded<oi::OiJob>& job,
    Ret (oi::OiJob::*slot)(Args...) const,
    Qt::ConnectionType type = Qt::AutoConnection) {
  const QMetaObject::Connection c = QObject::connect(
      sender, signal, sender,
      [job, slot](Args... args) {
        if (!job.isNull()) (job.data()->*slot)(args...);
      },
      type);
  rememberJobConnection(sender, c);
  return c;
}

#endif  // JOBCONNECT_H

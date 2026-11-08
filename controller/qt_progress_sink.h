#ifndef QTPROGRESSSINK_H
#define QTPROGRESSSINK_H

#include <QObject>
#include <QString>

#include "progress.h"
#include "qt_executor.h"

/*!
 * The oi::ProgressSink of the Qt application. Operations report from whatever
 * thread they run on (an exchange plugin on the exchange thread, for one);
 * this posts every report onto the thread the sink lives in (the UI thread)
 * through a QtExecutor, in order, and re-emits it as a signal there (a report
 * made on that thread already is emitted directly).
 *
 * `percent` is 0..100, or -1 when the operation cannot say how far it is (a
 * busy indicator). Reports still queued when the sink is destroyed are
 * discarded.
 */
class QtProgressSink : public QObject, public oi::ProgressSink {
  Q_OBJECT

 public:
  explicit QtProgressSink(QObject* parent = nullptr);

  void onProgress(const oi::Progress& progress) override;

 signals:
  void progressChanged(int percent, const QString& message);

 private:
  QtExecutor executor;
};

#endif  // QTPROGRESSSINK_H

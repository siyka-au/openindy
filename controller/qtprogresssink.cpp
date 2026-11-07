#include "qtprogresssink.h"

#include <QThread>
#include <cmath>
#include <utility>

#include "qtconvert.h"

QtProgressSink::QtProgressSink(QObject* parent) : QObject(parent) {}

void QtProgressSink::onProgress(const oi::Progress& progress) {
  const int percent =
      progress.isDeterminate()
          ? static_cast<int>(std::lround(progress.fraction * 100.0))
          : -1;
  QString message = toQString(progress.message);
  if (QThread::currentThread() == this->thread()) {
    emit this->progressChanged(percent, message);
    return;
  }
  executor.post([this, percent, message = std::move(message)] {
    emit this->progressChanged(percent, message);
  });
}

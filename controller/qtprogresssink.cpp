#include "qtprogresssink.h"

#include <cmath>

#include "qtconvert.h"

QtProgressSink::QtProgressSink(QObject* parent) : QObject(parent) {}

void QtProgressSink::onProgress(const oi::Progress& progress) {
  const int percent =
      progress.isDeterminate()
          ? static_cast<int>(std::lround(progress.fraction * 100.0))
          : -1;
  executor.post([this, percent, message = toQString(progress.message)] {
    emit this->progressChanged(percent, message);
  });
}

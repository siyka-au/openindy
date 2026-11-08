#include "qt_executor.h"

#include <utility>

void QtExecutor::post(std::function<void()> task) {
  if (!task) {
    return;
  }
  QMetaObject::invokeMethod(this, std::move(task), Qt::QueuedConnection);
}

#include "qtfunctionadapter.h"

#include <QHash>
#include <QMutex>
#include <QMutexLocker>
#include <QThread>

#include "function.h"

using namespace oi;

namespace {
QMutex& registryMutex() {
  static QMutex mutex;
  return mutex;
}
QHash<const Function*, QtFunctionAdapter*>& registry() {
  static QHash<const Function*, QtFunctionAdapter*> r;
  return r;
}
}  // namespace

QtFunctionAdapter* QtFunctionAdapter::of(Function* function) {
  if (function == nullptr) {
    return nullptr;
  }
  QMutexLocker locker(&registryMutex());
  if (auto it = registry().constFind(function); it != registry().constEnd()) {
    return it.value();
  }
  auto* adapter = new QtFunctionAdapter;
  // no QObject parent: the function may be used from another thread than the
  // one asking for the adapter, so move it next to the function instead
  if (adapter->thread() != function->thread()) {
    adapter->moveToThread(function->thread());
  }
  registry().insert(function, adapter);
  function->setObserver(adapter);
  return adapter;
}

QtFunctionAdapter* QtFunctionAdapter::of(const QPointer<Function>& function) {
  return of(function.data());
}

int QtFunctionAdapter::registrySize() {
  QMutexLocker locker(&registryMutex());
  return static_cast<int>(registry().size());
}

QtFunctionAdapter::~QtFunctionAdapter() {
  QMutexLocker locker(&registryMutex());
  for (auto it = registry().begin(); it != registry().end();) {
    it = (it.value() == this) ? registry().erase(it) : std::next(it);
  }
}

// the function is going away: forget it and end the adapter's life with it
void QtFunctionAdapter::onFunctionDestroyed(Function* function) {
  {
    QMutexLocker locker(&registryMutex());
    registry().remove(function);
  }
  function->setObserver(nullptr);
  if (QThread::currentThread() == this->thread()) {
    delete this;
  } else {
    this->deleteLater();
  }
}

void QtFunctionAdapter::onInputElementsChanged() {
  emit this->inputElementsChanged();
}

void QtFunctionAdapter::onScalarInputParametersChanged() {
  emit this->scalarInputParametersChanged();
}

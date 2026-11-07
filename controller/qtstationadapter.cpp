#include "qtstationadapter.h"

#include <QCoreApplication>
#include <QHash>
#include <QMutex>
#include <QMutexLocker>
#include <QThread>

#include "station.h"

using namespace oi;

namespace {
QMutex& registryMutex() {
  static QMutex mutex;
  return mutex;
}
QHash<const Station*, QtStationAdapter*>& registry() {
  static QHash<const Station*, QtStationAdapter*> r;
  return r;
}
}  // namespace

QtStationAdapter* QtStationAdapter::of(Station* station) {
  if (station == nullptr) {
    return nullptr;
  }
  QMutexLocker locker(&registryMutex());
  if (auto it = registry().constFind(station); it != registry().constEnd()) {
    return it.value();
  }
  auto* adapter = new QtStationAdapter;
  // no QObject parent: the station may be used from any thread, but the adapter
  // re-emits Qt signals for the UI, so it lives on the application's thread
  if (QCoreApplication::instance() != nullptr &&
      adapter->thread() != QCoreApplication::instance()->thread()) {
    adapter->moveToThread(QCoreApplication::instance()->thread());
  }
  registry().insert(station, adapter);
  station->setStationObserver(adapter);
  return adapter;
}

QtStationAdapter* QtStationAdapter::of(const oi::Guarded<Station>& station) {
  return of(station.data());
}

int QtStationAdapter::registrySize() {
  QMutexLocker locker(&registryMutex());
  return static_cast<int>(registry().size());
}

QtStationAdapter::~QtStationAdapter() {
  QMutexLocker locker(&registryMutex());
  for (auto it = registry().begin(); it != registry().end();) {
    it = (it.value() == this) ? registry().erase(it) : std::next(it);
  }
}

// the station is going away: forget it and end the adapter's life with it
void QtStationAdapter::onStationDestroyed(Station* station) {
  {
    QMutexLocker locker(&registryMutex());
    registry().remove(station);
  }
  station->setStationObserver(nullptr);
  if (QThread::currentThread() == this->thread()) {
    delete this;
  } else {
    this->deleteLater();
  }
}

void QtStationAdapter::onRealTimeReading(const QVariantMap& reading) {
  emit this->realTimeReading(reading);
}

void QtStationAdapter::onRealTimeStatus(const QMap<QString, QString>& status) {
  emit this->realTimeStatus(status);
}

void QtStationAdapter::onConnectionLost() { emit this->connectionLost(); }

void QtStationAdapter::onConnectionReceived() {
  emit this->connectionReceived();
}

void QtStationAdapter::onIsReadyForMeasurement(bool isReady) {
  emit this->isReadyForMeasurement(isReady);
}

void QtStationAdapter::onCommandFinished(bool success, const QString& msg,
                                         SensorAction sensorAction) {
  emit this->commandFinished(success, msg, sensorAction);
}

void QtStationAdapter::onMeasurementFinished(
    int geomId, const QList<oi::Guarded<Reading>>& readings) {
  emit this->measurementFinished(geomId, readings);
}

void QtStationAdapter::onMeasurementDone(bool success) {
  emit this->measurementDone(success);
}

void QtStationAdapter::onSensorMessage(const QString& msg, MessageTypes msgType,
                                       MessageDestinations msgDest) {
  emit this->sensorMessage(msg, msgType, msgDest);
}

void QtStationAdapter::onSensorStatus(SensorStatus status, const QString& msg) {
  emit this->sensorStatus(status, msg);
}

#ifndef QTSTATIONADAPTER_H
#define QTSTATIONADAPTER_H

#include <QList>
#include <QMap>
#include <QObject>
#include <QPointer>
#include <QVariantMap>
#include <map>
#include <vector>

#include "stationobserver.h"

namespace oi {
class Reading;
class Station;
}  // namespace oi

/*!
 * Turns the plain oi::StationObserver callbacks of a Station back into the Qt
 * signals the Qt application connects to. There is exactly one adapter per
 * station, created on first use by of() and destroyed together with the
 * station. The signals carry the names the station used to declare itself.
 *
 * The signals are emitted on whatever thread the station's executor delivers
 * on: with the application's QtExecutor that is the UI thread.
 */
class QtStationAdapter : public QObject, public oi::StationObserver {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(QtStationAdapter)

 public:
  // the adapter of `station` (created and registered on first use), or nullptr
  // for a null station. The adapter lives in the station's thread.
  static QtStationAdapter* of(oi::Station* station);
  static QtStationAdapter* of(const oi::Guarded<oi::Station>& station);

  // number of live adapters (for tests)
  static int registrySize();

  ~QtStationAdapter() override;

 signals:

  // real time data
  void realTimeReading(QVariantMap reading);
  void realTimeStatus(std::map<QString, QString> status);

  // connection information
  void connectionLost();
  void connectionReceived();

  // ready state
  void isReadyForMeasurement(bool isReady);

  // sensor action callbacks
  void commandFinished(bool success, QString msg,
                       oi::SensorAction sensorAction);
  void measurementFinished(int geomId,
                           std::vector<oi::Guarded<oi::Reading>> readings);
  void measurementDone(bool success);

  // what the driver reports
  void sensorMessage(QString msg, oi::MessageTypes msgType,
                     oi::MessageDestinations msgDest);
  void sensorStatus(oi::SensorStatus status, QString msg);

 private:
  QtStationAdapter() = default;

  void onStationDestroyed(oi::Station* station) override;
  void onRealTimeReading(const oi::ValueMap& reading) override;
  void onRealTimeStatus(
      const std::map<std::string, std::string>& status) override;
  void onConnectionLost() override;
  void onConnectionReceived() override;
  void onIsReadyForMeasurement(bool isReady) override;
  void onCommandFinished(bool success, const std::string& msg,
                         oi::SensorAction sensorAction) override;
  void onMeasurementFinished(
      int geomId,
      const std::vector<oi::Guarded<oi::Reading>>& readings) override;
  void onMeasurementDone(bool success) override;
  void onSensorMessage(const std::string& msg, oi::MessageTypes msgType,
                       oi::MessageDestinations msgDest) override;
  void onSensorStatus(oi::SensorStatus status, const std::string& msg) override;
};

#endif  // QTSTATIONADAPTER_H

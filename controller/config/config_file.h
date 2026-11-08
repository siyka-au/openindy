#ifndef OI_APP_CONTROLLER_CONFIG_CONFIG_FILE_H_
#define OI_APP_CONTROLLER_CONFIG_CONFIG_FILE_H_

// Reading and writing the JSON config files (see config_codec.h for the
// format). Header-only so the config managers share one implementation.

#include <QByteArray>
#include <QFile>
#include <QString>
#include <string>

#include "config_codec.h"

namespace configfile {

/*! Writes the text, replacing the file. False if it cannot be written. */
inline bool write(const QString& path, const std::string& json) {
  QFile file(path);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    return false;
  }
  const QByteArray bytes = QByteArray::fromStdString(json);
  return file.write(bytes) == bytes.size();
}

/*! The measurement config in the file, empty if it is missing or invalid. */
inline std::optional<oi::MeasurementConfig> readMeasurementConfig(
    const QString& path) {
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly)) {
    return std::nullopt;
  }
  return oi::codec::decodeMeasurementConfig(file.readAll().toStdString());
}

/*! The sensor configuration in the file, empty if it is missing or invalid. */
inline std::optional<oi::SensorConfiguration> readSensorConfiguration(
    const QString& path) {
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly)) {
    return std::nullopt;
  }
  return oi::codec::decodeSensorConfiguration(file.readAll().toStdString());
}

}  // namespace configfile

#endif  // OI_APP_CONTROLLER_CONFIG_CONFIG_FILE_H_

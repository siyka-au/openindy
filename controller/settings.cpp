#include "settings.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <map>

#include "containers.h"

bool Settings::isInit = false;
std::map<QString, QString> Settings::systemLayer;
std::map<QString, QString> Settings::userLayer;
std::map<QString, QString> Settings::projectLayer;

/*!
 * \brief Settings::init
 * Loads the System layer (config/defaults.json) and User layer
 * (~/.openindy/settings.json, if present) once.
 */
void Settings::init() {
  Settings::loadSystemLayer();
  Settings::loadUserLayer();
  Settings::isInit = true;
}

/*!
 * \brief Settings::get
 * Resolves key through Project -> User -> System, first hit wins.
 * \param key
 * \return the resolved value, or an empty string if unset at every tier
 */
QString Settings::get(const QString& key) {
  if (!Settings::isInit) {
    Settings::init();
  }

  if (Settings::projectLayer.contains(key)) {
    return oi::value(Settings::projectLayer, key);
  }
  if (Settings::userLayer.contains(key)) {
    return oi::value(Settings::userLayer, key);
  }
  if (Settings::systemLayer.contains(key)) {
    return oi::value(Settings::systemLayer, key);
  }

  return QString();
}

/*!
 * \brief Settings::setUserValue
 * Persists a User-tier override to ~/.openindy/settings.json.
 * \param key
 * \param value
 */
void Settings::setUserValue(const QString& key, const QString& value) {
  if (!Settings::isInit) {
    Settings::init();
  }
  Settings::userLayer.insert_or_assign(key, value);
  Settings::saveUserLayer();
}

/*!
 * \brief Settings::setProjectValue
 * Project-tier override - in-memory only, cleared when the project closes.
 * \param key
 * \param value
 */
void Settings::setProjectValue(const QString& key, const QString& value) {
  if (!Settings::isInit) {
    Settings::init();
  }
  Settings::projectLayer.insert_or_assign(key, value);
}

/*!
 * \brief Settings::clearProjectValues
 */
void Settings::clearProjectValues() { Settings::projectLayer.clear(); }

/*!
 * \brief Settings::loadSystemLayer
 * Parses config/defaults.json's createFeature triples into the flat
 * "defaultFunction.<featureType>" / "defaultMeasurementConfig.<featureType>"
 * key space. Values are bare names, not full plugin references - resolved
 * against whichever repository owns them at read time (see PluginRepository,
 * MeasurementConfigRepository), the same "convenience lookup, first match"
 * philosophy already used for getByName() elsewhere.
 */
void Settings::loadSystemLayer() {
  Settings::loadSystemLayerFromFile(qApp->applicationDirPath() +
                                    "/config/defaults.json");
}

/*!
 * \brief Settings::loadSystemLayerFromFile
 * \param path
 */
void Settings::loadSystemLayerFromFile(const QString& path) {
  Settings::systemLayer.clear();

  QFile file(path);
  if (!file.open(QFile::ReadOnly | QFile::Text)) {
    return;
  }

  QJsonParseError parseError;
  QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
  if (parseError.error != QJsonParseError::NoError) {
    return;
  }

  QJsonObject config = doc.object();
  foreach (const QJsonValue& v, config.value("createFeature").toArray()) {
    QJsonObject c = v.toObject();
    QString featureType = c.value("featureType").toString();
    QString function = c.value("function").toString();
    QString measurementConfig = c.value("measurementConfig").toString();
    if (featureType.isEmpty()) {
      continue;
    }
    if (!function.isEmpty()) {
      Settings::systemLayer.insert_or_assign("defaultFunction." + featureType,
                                             function);
    }
    if (!measurementConfig.isEmpty()) {
      Settings::systemLayer.insert_or_assign(
          "defaultMeasurementConfig." + featureType, measurementConfig);
    }
  }

  Settings::isInit = true;
}

/*!
 * \brief Settings::loadUserLayer
 * ~/.openindy/settings.json is a plain flat {"key": "value", ...} object.
 */
void Settings::loadUserLayer() {
  Settings::userLayer.clear();

  QFile file(Settings::userSettingsFilePath());
  if (!file.open(QFile::ReadOnly | QFile::Text)) {
    return;
  }

  QJsonParseError parseError;
  QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
  if (parseError.error != QJsonParseError::NoError) {
    return;
  }

  QJsonObject obj = doc.object();
  foreach (const QString& key, obj.keys()) {
    Settings::userLayer.insert_or_assign(key, obj.value(key).toString());
  }
}

/*!
 * \brief Settings::saveUserLayer
 */
void Settings::saveUserLayer() {
  QString path = Settings::userSettingsFilePath();
  QDir().mkpath(QFileInfo(path).absolutePath());

  QJsonObject obj;
  foreach (const QString& key, oi::keys(Settings::userLayer)) {
    obj.insert(key, oi::value(Settings::userLayer, key));
  }

  QFile file(path);
  if (file.open(QFile::WriteOnly | QFile::Text | QFile::Truncate)) {
    file.write(QJsonDocument(obj).toJson());
    file.close();
  }
}

/*!
 * \brief Settings::userSettingsFilePath
 * \return ~/.openindy/settings.json
 */
QString Settings::userSettingsFilePath() {
  return QDir::homePath() + "/.openindy/settings.json";
}

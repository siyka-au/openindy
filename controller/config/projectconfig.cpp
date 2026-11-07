#include "projectconfig.h"

#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <optional>

QString ProjectConfig::configFolder;
QString ProjectConfig::importNominalPath;
QString ProjectConfig::projectPath;

int ProjectConfig::metricUnit;
int ProjectConfig::angularUnit;
int ProjectConfig::temperatureUnit;

int ProjectConfig::distanceDigits;
int ProjectConfig::angularDigits;
int ProjectConfig::temperatureDigits;

bool ProjectConfig::useSounds;
int ProjectConfig::autoSaveInterval;

QList<oi::FeatureDisplayAttributes> ProjectConfig::displayColumns;

/*!
 * \brief ProjectConfig::getImportNominalPath
 * \return
 */
QString ProjectConfig::getImportNominalPath() { return importNominalPath; }

/*!
 * \brief ProjectConfig::setImportNominalPath
 * \param value
 */
void ProjectConfig::setImportNominalPath(const QString& value) {
  importNominalPath = value;
}

/*!
 * \brief ProjectConfig::getProjectPath
 * \return
 */
QString ProjectConfig::getProjectPath() { return projectPath; }

/*!
 * \brief ProjectConfig::setProjectPath
 * \param value
 */
void ProjectConfig::setProjectPath(const QString& value) {
  projectPath = value;
}

/*!
 * \brief ProjectConfig::getMetricUnit
 * \return
 */
int ProjectConfig::getMetricUnit() { return metricUnit; }

/*!
 * \brief ProjectConfig::setMetricUnit
 * \param value
 */
void ProjectConfig::setMetricUnit(int value) {
  metricUnit = value;
  saveprojectSettingsConfigFile();
}

/*!
 * \brief ProjectConfig::getAngularUnit
 * \return
 */
int ProjectConfig::getAngularUnit() { return angularUnit; }

/*!
 * \brief ProjectConfig::setAngularUnit
 * \param value
 */
void ProjectConfig::setAngularUnit(int value) {
  angularUnit = value;
  saveprojectSettingsConfigFile();
}

/*!
 * \brief ProjectConfig::getTemperatureUnit
 * \return
 */
int ProjectConfig::getTemperatureUnit() { return temperatureUnit; }

/*!
 * \brief ProjectConfig::setTemperatureUnit
 * \param value
 */
void ProjectConfig::setTemperatureUnit(int value) { temperatureUnit = value; }

/*!
 * \brief ProjectConfig::getDistanceDigits
 * \return
 */
int ProjectConfig::getDistanceDigits() { return distanceDigits; }

/*!
 * \brief ProjectConfig::setDistanceDigits
 * \param value
 */
void ProjectConfig::setDistanceDigits(int value) { distanceDigits = value; }

/*!
 * \brief ProjectConfig::getAngularDigits
 * \return
 */
int ProjectConfig::getAngularDigits() { return angularDigits; }

/*!
 * \brief ProjectConfig::setAngularDigits
 * \param value
 */
void ProjectConfig::setAngularDigits(int value) { angularDigits = value; }

/*!
 * \brief ProjectConfig::getTemperatureDigits
 * \return
 */
int ProjectConfig::getTemperatureDigits() { return temperatureDigits; }

/*!
 * \brief ProjectConfig::setTemperatureDigits
 * \param value
 */
void ProjectConfig::setTemperatureDigits(int value) {
  temperatureDigits = value;
}

/*!
 * \brief ProjectConfig::getUseSounds
 * \return
 */
bool ProjectConfig::getUseSounds() { return useSounds; }

/*!
 * \brief ProjectConfig::setUseSounds
 * \param value
 */
void ProjectConfig::setUseSounds(bool value) { useSounds = value; }

int ProjectConfig::getAutoSaveInterval() { return autoSaveInterval; }

void ProjectConfig::setAutoSaveInterval(int value) { autoSaveInterval = value; }

QList<oi::FeatureDisplayAttributes> ProjectConfig::getDisplayColumns() {
  return displayColumns;
}

namespace {

// The column names as they appear in projectSettingsConfig.json. TODO derive
// these from the enum; not possible while the enums are not Q_ENUM.
struct ColumnName {
  const char* name;
  oi::FeatureDisplayAttributes attribute;
};

constexpr ColumnName columnNames[] = {
    {"Type", oi::FeatureDisplayAttributes::eFeatureDisplayType},
    {"Name", oi::FeatureDisplayAttributes::eFeatureDisplayName},
    {"Comment", oi::FeatureDisplayAttributes::eFeatureDisplayComment},
    {"Group", oi::FeatureDisplayAttributes::eFeatureDisplayGroup},
    {"IsSolved", oi::FeatureDisplayAttributes::eFeatureDisplayIsSolved},
    {"IsUpdated", oi::FeatureDisplayAttributes::eFeatureDisplayIsUpdated},
    {"Functions", oi::FeatureDisplayAttributes::eFeatureDisplayFunctions},
    {"UsedFor", oi::FeatureDisplayAttributes::eFeatureDisplayUsedFor},
    {"PreviouslyNeeded",
     oi::FeatureDisplayAttributes::eFeatureDisplayPreviouslyNeeded},
    {"StDev", oi::FeatureDisplayAttributes::eFeatureDisplayStDev},
    {"MeasurementConfig",
     oi::FeatureDisplayAttributes::eFeatureDisplayMeasurementConfig},
    {"Observations", oi::FeatureDisplayAttributes::eFeatureDisplayObservations},
    {"IsCommon", oi::FeatureDisplayAttributes::eFeatureDisplayIsCommon},
    {"IsActual", oi::FeatureDisplayAttributes::eFeatureDisplayIsActual},
    {"X", oi::FeatureDisplayAttributes::eFeatureDisplayX},
    {"Y", oi::FeatureDisplayAttributes::eFeatureDisplayY},
    {"Z", oi::FeatureDisplayAttributes::eFeatureDisplayZ},
    {"PrimaryI", oi::FeatureDisplayAttributes::eFeatureDisplayPrimaryI},
    {"PrimaryJ", oi::FeatureDisplayAttributes::eFeatureDisplayPrimaryJ},
    {"PrimaryK", oi::FeatureDisplayAttributes::eFeatureDisplayPrimaryK},
    {"RadiusA", oi::FeatureDisplayAttributes::eFeatureDisplayRadiusA},
    {"RadiusB", oi::FeatureDisplayAttributes::eFeatureDisplayRadiusB},
    {"SecondaryI", oi::FeatureDisplayAttributes::eFeatureDisplaySecondaryI},
    {"SecondaryJ", oi::FeatureDisplayAttributes::eFeatureDisplaySecondaryJ},
    {"SecondaryK", oi::FeatureDisplayAttributes::eFeatureDisplaySecondaryK},
    {"Aperture", oi::FeatureDisplayAttributes::eFeatureDisplayAperture},
    {"A", oi::FeatureDisplayAttributes::eFeatureDisplayA},
    {"B", oi::FeatureDisplayAttributes::eFeatureDisplayB},
    {"C", oi::FeatureDisplayAttributes::eFeatureDisplayC},
    {"Angle", oi::FeatureDisplayAttributes::eFeatureDisplayAngle},
    {"Distance", oi::FeatureDisplayAttributes::eFeatureDisplayDistance},
    {"MeasurementSeries",
     oi::FeatureDisplayAttributes::eFeatureDisplayMeasurementSeries},
    {"Temperature", oi::FeatureDisplayAttributes::eFeatureDisplayTemperature},
    {"Length", oi::FeatureDisplayAttributes::eFeatureDisplayLength},
    {"ExpansionOriginX",
     oi::FeatureDisplayAttributes::eFeatureDisplayExpansionOriginX},
    {"ExpansionOriginY",
     oi::FeatureDisplayAttributes::eFeatureDisplayExpansionOriginY},
    {"ExpansionOriginZ",
     oi::FeatureDisplayAttributes::eFeatureDisplayExpansionOriginZ},
    {"FormError", oi::FeatureDisplayAttributes::eFeatureDisplayFormError},
};

QString configPath(const QString& folder, const QString& fileName) {
  const QString base =
      folder.isEmpty()
          ? QCoreApplication::applicationDirPath() + "/config/projectConfigs"
          : folder;
  return base + "/" + fileName;
}

// The root object of a JSON file; empty if the file is missing, unreadable or
// not a JSON object.
std::optional<QJsonObject> readObject(const QString& path) {
  QFile file(path);
  if (!QFileInfo(file).exists() || !file.open(QIODevice::ReadOnly)) {
    return std::nullopt;
  }
  const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
  if (!doc.isObject()) {
    return std::nullopt;
  }
  return doc.object();
}

// Only overwrites a file that already exists, as before: the config folder is
// part of the installation.
bool writeObject(const QString& path, const QJsonObject& root) {
  QFile file(path);
  if (!QFileInfo(file).exists()) {
    return false;
  }
  if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    return false;
  }
  const QByteArray bytes = QJsonDocument(root).toJson(QJsonDocument::Indented);
  return file.write(bytes) == bytes.size();
}

}  // namespace

void ProjectConfig::setConfigFolder(const QString& folder) {
  configFolder = folder;
}

/*!
 * \brief ProjectConfig::loadProjectPathConfigFile
 * \return false if the file is missing or invalid; the paths are then left
 * as they were
 */
bool ProjectConfig::loadProjectPathConfigFile() {
  const auto root =
      readObject(configPath(configFolder, "projectPathConfig.json"));
  if (!root) {
    return false;
  }
  const QJsonValue importPath = root->value("importNominalPath");
  const QJsonValue project = root->value("projectPath");
  if (!importPath.isString() || !project.isString()) {
    return false;
  }
  importNominalPath = importPath.toString();
  projectPath = project.toString();
  return true;
}

/*!
 * \brief ProjectConfig::saveProjectPathConfigFile
 * \return
 */
bool ProjectConfig::saveProjectPathConfigFile() {
  QJsonObject root;
  root["importNominalPath"] = importNominalPath;
  root["projectPath"] = projectPath;
  return writeObject(configPath(configFolder, "projectPathConfig.json"), root);
}

/*!
 * \brief ProjectConfig::loadProjectSettingsConfigFile
 * \return false if the file is missing or invalid
 */
bool ProjectConfig::loadProjectSettingsConfigFile() {
  const auto root =
      readObject(configPath(configFolder, "projectSettingsConfig.json"));
  if (!root) {
    return false;
  }

  // every one of these is required, as before
  const char* const required[] = {
      "projectDistanceUnit",    "projectAngularUnit",
      "projectTemperatureUnit", "projectDistanceDigits",
      "projectAngularDigits",   "projectTemperatureDigits",
      "projectSoundSettings"};
  for (const char* key : required) {
    if (!root->value(key).isDouble()) {
      return false;
    }
  }
  metricUnit = root->value("projectDistanceUnit").toInt();
  angularUnit = root->value("projectAngularUnit").toInt();
  temperatureUnit = root->value("projectTemperatureUnit").toInt();
  distanceDigits = root->value("projectDistanceDigits").toInt();
  angularDigits = root->value("projectAngularDigits").toInt();
  temperatureDigits = root->value("projectTemperatureDigits").toInt();
  useSounds = root->value("projectSoundSettings").toInt() != 0;

  // default minutes when absent
  autoSaveInterval = root->value("projectAutoSaveInterval").toInt(10);

  displayColumns.clear();
  const QJsonArray columns = root->value("projectFeatureViewSettings")
                                 .toObject()
                                 .value("columns")
                                 .toArray();
  for (const QJsonValue& column : columns) {
    const QString name = column.toObject().value("name").toString();
    for (const ColumnName& known : columnNames) {
      if (QString::compare(name, QLatin1String(known.name),
                           Qt::CaseInsensitive) == 0) {
        displayColumns.append(known.attribute);
        break;
      }
    }
  }

  return !displayColumns.isEmpty();  // no display columns found: invalid
}

/*!
 * \brief ProjectConfig::saveprojectSettingsConfigFile
 * \return
 */
bool ProjectConfig::saveprojectSettingsConfigFile() {
  QJsonObject root;
  root["projectDistanceUnit"] = metricUnit;
  root["projectAngularUnit"] = angularUnit;
  root["projectTemperatureUnit"] = temperatureUnit;
  root["projectDistanceDigits"] = distanceDigits;
  root["projectAngularDigits"] = angularDigits;
  root["projectTemperatureDigits"] = temperatureDigits;
  root["projectSoundSettings"] = useSounds ? 1 : 0;
  root["projectAutoSaveInterval"] = autoSaveInterval;

  QJsonArray columns;
  for (const oi::FeatureDisplayAttributes attribute : displayColumns) {
    for (const ColumnName& known : columnNames) {
      if (known.attribute == attribute) {
        columns.append(QJsonObject{{"name", QLatin1String(known.name)}});
        break;
      }
    }
  }
  root["projectFeatureViewSettings"] = QJsonObject{{"columns", columns}};

  return writeObject(configPath(configFolder, "projectSettingsConfig.json"),
                     root);
}

// see SettingsDialog::initGUI()
// see SettingsDialog::updateDisplayConfigFromSelection()
ParameterDisplayConfig ProjectConfig::getParameterDisplayConfig() {
  ParameterDisplayConfig config;
  config.setDisplayDigits(DimensionType::eAngular,
                          ProjectConfig::angularDigits);
  config.setDisplayDigits(DimensionType::eMetric,
                          ProjectConfig::distanceDigits);
  config.setDisplayDigits(DimensionType::eTemperature,
                          ProjectConfig::temperatureDigits);
  config.setDisplayUnitType(
      DimensionType::eAngular,
      static_cast<oi::UnitType>(ProjectConfig::angularUnit));
  config.setDisplayUnitType(
      DimensionType::eMetric,
      static_cast<oi::UnitType>(ProjectConfig::metricUnit));
  config.setDisplayUnitType(
      DimensionType::eTemperature,
      static_cast<oi::UnitType>(ProjectConfig::temperatureUnit));
  config.setUseSounds(ProjectConfig::useSounds);

  return config;
}

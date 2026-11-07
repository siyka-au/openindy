#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

#include "config_codec.h"
#include "configfile.h"
#include "containers.h"
#include "projectconfig.h"
#include "sensortype.h"

using namespace oi;

/*!
 * The human-editable config files are JSON (protobuf JSON mapping for the
 * measurement and sensor configs, plain Qt JSON for the project settings).
 * This loads every file the application ships and checks it says what the old
 * XML said, and that missing or corrupt files fall back instead of crashing.
 */
class ConfigFilesTest : public QObject {
  Q_OBJECT

 private slots:
  void shippedMeasurementConfigs();
  void shippedSensorConfiguration();
  void everyShippedFileIsJson();
  void configFileHelpersRoundTrip();
  void configFileHelpersRejectBadFiles();
  void shippedProjectConfigs();
  void projectConfigRoundTrip();
  void projectConfigFallsBackOnBadFiles();

 private:
  static QString shipped(const QString& relative) {
    return QStringLiteral(RESDIR) + "/" + relative;
  }
  static bool copyShipped(const QString& name, const QDir& to) {
    return QFile::copy(shipped("projectConfigs/" + name),
                       to.absoluteFilePath(name));
  }
  static void writeFile(const QString& path, const QByteArray& bytes) {
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(bytes);
  }
};

void ConfigFilesTest::shippedMeasurementConfigs() {
  const auto stdScan = configfile::readMeasurementConfig(
      shipped("measurementConfigs/StdScan.json"));
  QVERIFY(stdScan.has_value());
  QCOMPARE(stdScan->getName(), QString("StdScan"));
  QVERIFY(stdScan->getMeasurementMode() ==
          MeasurementModes::eFast_MeasurementMode);
  QVERIFY(stdScan->getMeasurementType() ==
          MeasurementTypes::eScanDistanceDependent_MeasurementType);
  QVERIFY(!stdScan->getMeasureTwoSides());
  QCOMPARE(stdScan->getMaxObservations(), 143);
  QCOMPARE(stdScan->getTimeInterval(), 0.0);
  QCOMPARE(stdScan->getDistanceInterval(), 20.0);
  QVERIFY(!stdScan->getSensorType().isValid());

  const auto axis = configfile::readMeasurementConfig(
      shipped("measurementConfigs/Achsverlauf.json"));
  QVERIFY(axis.has_value());
  QCOMPARE(axis->getName(), QString("Achsverlauf"));
  QCOMPARE(axis->getMaxObservations(), 125);
  QCOMPARE(axis->getDistanceInterval(), 20.0);
  QVERIFY(axis->getMeasurementType() ==
          MeasurementTypes::eScanDistanceDependent_MeasurementType);
}

void ConfigFilesTest::shippedSensorConfiguration() {
  const auto config = configfile::readSensorConfiguration(
      shipped("sensorConfigs/VirtualTracker.json"));
  QVERIFY(config.has_value());
  QCOMPARE(config->getName(), QString("VirtualTracker"));
  QVERIFY(config->getIsSaved());
  QCOMPARE(config->getPluginName(), QString("OpenIndy Default Plugin"));
  QCOMPARE(config->getSensorName(), QString("PseudoTracker"));
  QVERIFY(config->getTypeOfSensor().isValid());
  QCOMPARE(config->getTypeOfSensor().id(), QString("laserTracker"));
  QVERIFY(config->getConnectionConfig().typeOfConnection ==
          ConnectionKind::network());

  const Accuracy& accuracy = config->getAccuracy();
  QCOMPARE(accuracy.sigmaZenith, 1.57e-6);
  QCOMPARE(accuracy.sigmaDistance, 2.5e-5);
  QCOMPARE(accuracy.sigmaTemp, 0.5);
  QCOMPARE(accuracy.sigmaXyz.getAt(1), 2.5e-5);
  QVERIFY(accuracy.sigmaUndefined.empty());

  QCOMPARE(config->getDoubleParameter().size(), 17);
  QCOMPARE(oi::value(config->getDoubleParameter(), "echo value"), -123.45);
  QCOMPARE(oi::value(config->getDoubleParameter(), "Aa1 [arc sec]"), 0.064);
  QCOMPARE(oi::value(config->getStringParameter(), "reading type"),
           std::string("polar"));
  QCOMPARE(oi::value(config->getStringParameter(), "active probe"),
           std::string("1.5''"));
  QCOMPARE(oi::values(config->getAvailableStringParameter(), "reading type"),
           (std::vector<std::string>{"cartesian", "polar", "direction",
                                     "distance"}));
  QCOMPARE(
      oi::values(config->getAvailableStringParameter(), "active probe").size(),
      3);
}

void ConfigFilesTest::everyShippedFileIsJson() {
  for (const QString& folder :
       {"measurementConfigs", "sensorConfigs", "projectConfigs"}) {
    const QDir dir(shipped(folder));
    QVERIFY2(dir.exists(), qPrintable(folder));
    QVERIFY2(!dir.entryList({"*.json"}, QDir::Files).isEmpty(),
             qPrintable(folder));
    QVERIFY2(dir.entryList({"*.xml"}, QDir::Files).isEmpty(),
             qPrintable(folder));
  }
}

void ConfigFilesTest::configFileHelpersRoundTrip() {
  QTemporaryDir dir;
  QVERIFY(dir.isValid());

  MeasurementConfig config;
  config.setName("Mine");
  config.setMaxObservations(7);
  const QString path = dir.filePath("Mine.json");
  QVERIFY(configfile::write(path, codec::encodeMeasurementConfig(config)));
  const auto back = configfile::readMeasurementConfig(path);
  QVERIFY(back.has_value());
  QCOMPARE(back->getName(), QString("Mine"));
  QCOMPARE(back->getMaxObservations(), 7);

  SensorConfiguration sensor;
  sensor.setName("Mine");
  sensor.setSensorName("S");
  const QString sensorPath = dir.filePath("Sensor.json");
  QVERIFY(
      configfile::write(sensorPath, codec::encodeSensorConfiguration(sensor)));
  const auto sensorBack = configfile::readSensorConfiguration(sensorPath);
  QVERIFY(sensorBack.has_value());
  QCOMPARE(sensorBack->getSensorName(), QString("S"));
}

void ConfigFilesTest::configFileHelpersRejectBadFiles() {
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  QVERIFY(!configfile::readMeasurementConfig(dir.filePath("missing.json")));
  QVERIFY(!configfile::readSensorConfiguration(dir.filePath("missing.json")));

  // an old XML config is not a config any more
  writeFile(dir.filePath("old.json"),
            "<?xml version=\"1.0\"?><measurementConfig name=\"x\"/>");
  QVERIFY(!configfile::readMeasurementConfig(dir.filePath("old.json")));
  writeFile(dir.filePath("empty.json"), "");
  QVERIFY(!configfile::readSensorConfiguration(dir.filePath("empty.json")));
  writeFile(dir.filePath("binary.json"), QByteArray("\x00\xff\x01{", 4));
  QVERIFY(!configfile::readSensorConfiguration(dir.filePath("binary.json")));
}

void ConfigFilesTest::shippedProjectConfigs() {
  ProjectConfig::setConfigFolder(shipped("projectConfigs"));
  QVERIFY(ProjectConfig::loadProjectPathConfigFile());
  QCOMPARE(ProjectConfig::getImportNominalPath(), QString("D:/Daten/Desktop"));
  QCOMPARE(ProjectConfig::getProjectPath(), QString("D:/Daten/Desktop"));

  QVERIFY(ProjectConfig::loadProjectSettingsConfigFile());
  QCOMPARE(ProjectConfig::getMetricUnit(), 0);
  QCOMPARE(ProjectConfig::getAngularUnit(), 6);
  QCOMPARE(ProjectConfig::getTemperatureUnit(), 8);
  QCOMPARE(ProjectConfig::getDistanceDigits(), 2);
  QCOMPARE(ProjectConfig::getAngularDigits(), 6);
  QCOMPARE(ProjectConfig::getTemperatureDigits(), 1);
  QVERIFY(ProjectConfig::getUseSounds());
  QCOMPARE(ProjectConfig::getAutoSaveInterval(), 10);

  const auto columns = ProjectConfig::getDisplayColumns();
  QCOMPARE(columns.size(), 20);
  QVERIFY(columns.front() == FeatureDisplayAttributes::eFeatureDisplayType);
  QVERIFY(columns.back() == FeatureDisplayAttributes::eFeatureDisplayDistance);
  ProjectConfig::setConfigFolder(QString());
}

void ConfigFilesTest::projectConfigRoundTrip() {
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  QVERIFY(copyShipped("projectPathConfig.json", QDir(dir.path())));
  QVERIFY(copyShipped("projectSettingsConfig.json", QDir(dir.path())));
  ProjectConfig::setConfigFolder(dir.path());
  QVERIFY(ProjectConfig::loadProjectSettingsConfigFile());

  // the setters write the file back
  ProjectConfig::setMetricUnit(3);
  ProjectConfig::setAngularUnit(4);
  ProjectConfig::setAutoSaveInterval(25);
  ProjectConfig::setImportNominalPath("/tmp/\xC3\xA4 nominal \"quoted\"");
  ProjectConfig::setProjectPath("/tmp/projects");
  QVERIFY(ProjectConfig::saveprojectSettingsConfigFile());
  QVERIFY(ProjectConfig::saveProjectPathConfigFile());

  ProjectConfig::setMetricUnit(0);
  ProjectConfig::setAngularUnit(0);
  ProjectConfig::setAutoSaveInterval(0);
  ProjectConfig::setImportNominalPath("");
  ProjectConfig::setProjectPath("");
  // the setters above rewrote the file with zeros; restore the values
  ProjectConfig::setMetricUnit(3);
  ProjectConfig::setAngularUnit(4);
  ProjectConfig::setAutoSaveInterval(25);
  ProjectConfig::setImportNominalPath("/tmp/\xC3\xA4 nominal \"quoted\"");
  ProjectConfig::setProjectPath("/tmp/projects");
  QVERIFY(ProjectConfig::saveprojectSettingsConfigFile());
  QVERIFY(ProjectConfig::saveProjectPathConfigFile());

  QVERIFY(ProjectConfig::loadProjectSettingsConfigFile());
  QVERIFY(ProjectConfig::loadProjectPathConfigFile());
  QCOMPARE(ProjectConfig::getMetricUnit(), 3);
  QCOMPARE(ProjectConfig::getAngularUnit(), 4);
  QCOMPARE(ProjectConfig::getAutoSaveInterval(), 25);
  QCOMPARE(ProjectConfig::getImportNominalPath(),
           QString::fromUtf8("/tmp/\xC3\xA4 nominal \"quoted\""));
  QCOMPARE(ProjectConfig::getProjectPath(), QString("/tmp/projects"));
  QCOMPARE(ProjectConfig::getDisplayColumns().size(), 20);
  ProjectConfig::setConfigFolder(QString());
}

void ConfigFilesTest::projectConfigFallsBackOnBadFiles() {
  QTemporaryDir dir;
  QVERIFY(dir.isValid());
  ProjectConfig::setConfigFolder(dir.path());

  // missing files
  QVERIFY(!ProjectConfig::loadProjectPathConfigFile());
  QVERIFY(!ProjectConfig::loadProjectSettingsConfigFile());
  // saving does not create the file, as before
  QVERIFY(!ProjectConfig::saveProjectPathConfigFile());

  for (const QByteArray& garbage :
       {QByteArray(), QByteArray("not json"), QByteArray("[1, 2]"),
        QByteArray("{}"), QByteArray("{\"projectDistanceUnit\": \"x\"}"),
        QByteArray("<projectSettingsConfig/>"), QByteArray("\x00\xff", 2)}) {
    writeFile(dir.filePath("projectPathConfig.json"), garbage);
    writeFile(dir.filePath("projectSettingsConfig.json"), garbage);
    QVERIFY(!ProjectConfig::loadProjectPathConfigFile());
    QVERIFY(!ProjectConfig::loadProjectSettingsConfigFile());
  }

  // a settings file without display columns is invalid; one without an
  // auto-save interval gets the 10 minute default
  writeFile(dir.filePath("projectSettingsConfig.json"),
            R"({"projectDistanceUnit": 0, "projectAngularUnit": 1,
                "projectTemperatureUnit": 2, "projectDistanceDigits": 3,
                "projectAngularDigits": 4, "projectTemperatureDigits": 5,
                "projectSoundSettings": 0})");
  QVERIFY(!ProjectConfig::loadProjectSettingsConfigFile());
  QCOMPARE(ProjectConfig::getAutoSaveInterval(), 10);
  QCOMPARE(ProjectConfig::getAngularDigits(), 4);
  QVERIFY(!ProjectConfig::getUseSounds());

  writeFile(dir.filePath("projectSettingsConfig.json"),
            R"({"projectDistanceUnit": 0, "projectAngularUnit": 1,
                "projectTemperatureUnit": 2, "projectDistanceDigits": 3,
                "projectAngularDigits": 4, "projectTemperatureDigits": 5,
                "projectSoundSettings": 1,
                "projectFeatureViewSettings": {"columns": [
                  {"name": "name"}, {"name": "NoSuchColumn"}, 7, {}]}})");
  QVERIFY(ProjectConfig::loadProjectSettingsConfigFile());
  QCOMPARE(ProjectConfig::getDisplayColumns().size(), 1);
  QVERIFY(ProjectConfig::getDisplayColumns().front() ==
          FeatureDisplayAttributes::eFeatureDisplayName);
  ProjectConfig::setConfigFolder(QString());
}

QTEST_APPLESS_MAIN(ConfigFilesTest)
#include "tst_configfiles.moc"

#ifndef PROJECTCONFIG_H
#define PROJECTCONFIG_H

#include <QCoreApplication>
#include <QList>
#include <QString>
#include <vector>

#include "parameterdisplayconfig.h"
#include "types.h"

class ProjectConfig {
 public:
  static QString getImportNominalPath();
  static void setImportNominalPath(const QString& value);

  static QString getProjectPath();
  static void setProjectPath(const QString& value);

  static int getMetricUnit();
  static void setMetricUnit(int value);

  static int getAngularUnit();
  static void setAngularUnit(int value);

  static int getTemperatureUnit();
  static void setTemperatureUnit(int value);

  static int getDistanceDigits();
  static void setDistanceDigits(int value);

  static int getAngularDigits();
  static void setAngularDigits(int value);

  static int getTemperatureDigits();
  static void setTemperatureDigits(int value);

  static bool getUseSounds();
  static void setUseSounds(bool value);

  static int getAutoSaveInterval();
  static void setAutoSaveInterval(int value);

  static std::vector<oi::FeatureDisplayAttributes> getDisplayColumns();
  static ParameterDisplayConfig getParameterDisplayConfig();

  /*! Where the config files live; the application's own config/projectConfigs
   *  folder when empty (the default). Lets tests use a scratch folder. */
  static void setConfigFolder(const QString& folder);

  //********************************************
  // save and load functions for all config files
  //********************************************

  static bool loadProjectPathConfigFile();
  static bool saveProjectPathConfigFile();

  static bool loadProjectSettingsConfigFile();
  static bool saveprojectSettingsConfigFile();

 private:
  static QString configFolder;
  static QString importNominalPath;
  static QString projectPath;

  static int metricUnit;
  static int angularUnit;
  static int temperatureUnit;

  static int distanceDigits;
  static int angularDigits;
  static int temperatureDigits;

  static bool useSounds;

  static int autoSaveInterval;

  static std::vector<oi::FeatureDisplayAttributes> displayColumns;
};

#endif  // PROJECTCONFIG_H

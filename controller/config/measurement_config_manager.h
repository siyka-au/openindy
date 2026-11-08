#ifndef OI_APP_CONTROLLER_CONFIG_MEASUREMENT_CONFIG_MANAGER_H_
#define OI_APP_CONTROLLER_CONFIG_MEASUREMENT_CONFIG_MANAGER_H_

#include <QApplication>
#include <QMap>
#include <QObject>
#include <QString>
#include <map>
#include <vector>

#include "console.h"
#include "measurement_config.h"
#include "oi_job.h"
#include "plugin_repository.h"
#include "types.h"

#if defined(OI_MAIN_LIB)
#define OI_MAIN_EXPORT Q_DECL_EXPORT
#else
#define OI_MAIN_EXPORT Q_DECL_IMPORT
#endif

using namespace oi;

/*!
 * \brief The MeasurementConfigManager class
 * Used to save and manage all available measurement configs
 */
class OI_MAIN_EXPORT MeasurementConfigManager : public QObject {
  Q_OBJECT

 public:
  explicit MeasurementConfigManager(QObject* parent = nullptr);

  MeasurementConfigManager(const MeasurementConfigManager& copy,
                           QObject* parent = nullptr);

  MeasurementConfigManager& operator=(const MeasurementConfigManager& copy);

  // ###################################
  // get or set the current OpenIndy job
  // ###################################

  [[nodiscard]] const oi::Guarded<OiJob>& getCurrentJob() const;
  void setCurrentJob(const oi::Guarded<OiJob>& job);

  // ##############################
  // get or set measurement configs
  // ##############################

  // get configs
  [[nodiscard]] MeasurementConfig getConfig(
      const MeasurementConfigKey& key) const;
  [[nodiscard]] MeasurementConfig getUserConfig(const QString& name) const;
  [[nodiscard]] MeasurementConfig getProjectConfig(const QString& name) const;
  [[nodiscard]] MeasurementConfig getStandardConfig(const QString& name) const;
  [[nodiscard]] MeasurementConfig findConfig(const QString& name) const;
  [[nodiscard]] const std::vector<MeasurementConfig> getConfigs() const;
  [[nodiscard]] const std::vector<MeasurementConfig> getUserConfigs() const;
  [[nodiscard]] const std::vector<MeasurementConfig> getProjectConfigs() const;
  [[nodiscard]] const std::vector<QString> getUserConfigNames() const;
  [[nodiscard]] const std::vector<QString> getProjectConfigNames() const;

  // active config
  [[nodiscard]] MeasurementConfig getActiveConfig(
      const GeometryTypes& type) const;

  // add or remove configs
  void saveUserConfig(const MeasurementConfig& mConfig);
  void addProjectConfig(const MeasurementConfig& mConfig);
  void removeUserConfig(const QString& name);
  void removeProjectConfig(const QString& name);

  // replace a config
  void replaceMeasurementConfig(const MeasurementConfigKey& oldKey,
                                const MeasurementConfig& mConfig);

  void saveToConfigFolder(const QString& name,
                          const MeasurementConfig& mConfig);
  // load configs from the config folder
  void loadFromConfigFolder();

  // synchronize config manager
  void synchronize(const MeasurementConfigManager& other);

 signals:

  // #######################################
  // inform about measurement config changes
  // #######################################

  void measurementConfigurationsChanged();

  // ############
  // log messages
  // ############

  void sendMessage(const QString& msg, const MessageTypes& msgType,
                   const MessageDestinations& msgDest =
                       MessageDestinations::eConsoleMessage);

 private:
  // ##############
  // helper methods
  // ##############

  // save or remove configs
  void saveConfig(const MeasurementConfig& mConfig);
  void deleteMeasurementConfig(const QString& name);

  // job connects
  void connectJob();
  void disconnectJob();

  // ########################
  // save measurement configs
  // ########################

  std::map<MeasurementConfigKey, MeasurementConfig> configs;

  std::map<GeometryTypes, MeasurementConfig> activeMeasurementConfigs;

  // #################
  // helper attributes
  // #################

  oi::Guarded<OiJob> currentJob;
};

#endif  // OI_APP_CONTROLLER_CONFIG_MEASUREMENT_CONFIG_MANAGER_H_

#include "projectexchanger.h"

#include <memory>

#include "console.h"
#include "featureutil.h"
#include "pluginloader.h"
#include "pluginrepository.h"
#include "project_codec.h"
#include "qtconvert.h"
#include "version.h"

namespace {

// Creates plugin objects from the plugins installed next to the application,
// and answers measurement config lookups from the config manager.
class ApplicationHost : public oi::codec::ProjectHost {
 public:
  explicit ApplicationHost(const QPointer<MeasurementConfigManager>& configs)
      : configs_(configs) {}

  std::unique_ptr<Function> createFunction(const std::string& plugin,
                                           const std::string& name) override {
    return create<Function>(plugin, name, PluginLoader::loadFunctionPlugin);
  }

  std::unique_ptr<BundleAdjustment> createBundleAdjustment(
      const std::string& plugin, const std::string& name) override {
    return create<BundleAdjustment>(plugin, name,
                                    PluginLoader::loadBundleAdjustmentPlugin);
  }

  std::unique_ptr<Sensor> createSensor(const std::string& plugin,
                                       const std::string& name) override {
    return create<Sensor>(plugin, name, PluginLoader::loadSensorPlugin);
  }

  MeasurementConfig measurementConfig(
      const MeasurementConfigKey& key) const override {
    if (configs_.isNull()) {
      return {};
    }
    return configs_->getConfig(key);
  }

 private:
  template <class T, class Loader>
  static std::unique_ptr<T> create(const std::string& plugin,
                                   const std::string& name, Loader load) {
    const std::optional<sys::Plugin> found =
        PluginRepository::getByName(QString::fromStdString(plugin));
    if (!found.has_value()) {
      return nullptr;
    }
    oi::Guarded<T> object =
        load(found->filePath(), QString::fromStdString(name));
    return std::unique_ptr<T>(object.data());
  }

  QPointer<MeasurementConfigManager> configs_;
};

QString describeMissing(const std::vector<std::string>& missing) {
  QStringList names;
  for (const std::string& name : missing) {
    names << QString::fromStdString(name);
  }
  return names.join(", ");
}

}  // namespace

QByteArray ProjectExchanger::saveProject(const oi::Guarded<OiJob>& job) {
  if (job.isNull()) {
    Console::getInstance()->addLine("No project available",
                                    MessageTypes::eErrorMessage);
    return {};
  }
  const ApplicationHost host(this->mConfigManager);
  const std::string bytes =
      oi::codec::encodeProject(*job, host, OPENINDY_VERSION);
  return QByteArray(bytes.data(), static_cast<qsizetype>(bytes.size()));
}

ProjectExchanger::LoadResult ProjectExchanger::loadProject(
    const QByteArray& project) {
  ApplicationHost host(this->mConfigManager);
  oi::codec::DecodeResult decoded = oi::codec::decodeProject(
      std::string_view(project.constData(),
                       static_cast<std::size_t>(project.size())),
      host);

  LoadResult result;
  if (!decoded.ok()) {
    result.error = QString::fromStdString(decoded.error);
    if (!decoded.missingPlugins.empty()) {
      result.error +=
          QString(": %1").arg(describeMissing(decoded.missingPlugins));
    }
    return result;
  }

  // a project always has the initial bundle transformation
  if (decoded.job->getFeaturesByName("Bundle01_1").empty()) {
    FeatureUtil util;
    if (!util.createInitialBundleTransformation(decoded.job)) {
      delete decoded.job.data();
      result.error = "the project has no usable bundle system";
      return result;
    }
  }

  // the project's own measurement configs replace those of the previous one
  if (!this->mConfigManager.isNull()) {
    for (const MeasurementConfig& config :
         this->mConfigManager->getProjectConfigs()) {
      this->mConfigManager->removeProjectConfig(toQString(config.getName()));
    }
    for (const MeasurementConfig& config : decoded.projectConfigs) {
      if (config.isValid() && config.isProjectConfig() &&
          !this->mConfigManager->getStandardConfig(toQString(config.getName()))
               .isValid()) {
        this->mConfigManager->addProjectConfig(config);
      }
    }
  }

  result.job = decoded.job;
  return result;
}

QPointer<MeasurementConfigManager>&
ProjectExchanger::getMeasurementConfigManager() {
  return this->mConfigManager;
}

void ProjectExchanger::setMeasurementConfigManager(
    const QPointer<MeasurementConfigManager>& mConfigManager) {
  this->mConfigManager = mConfigManager;
}

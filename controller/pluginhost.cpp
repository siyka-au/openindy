#include "pluginhost.h"

#include <QJsonDocument>
#include <QMutex>
#include <QMutexLocker>
#include <QPluginLoader>
#include <map>
#include <memory>
#include <string>

#include "qtconvert.h"

namespace {

QMutex& cacheMutex() {
  static QMutex mutex;
  return mutex;
}

// Never destroyed on purpose: unloading plugin libraries during static
// destruction races with objects they created.
std::map<std::string, std::shared_ptr<oi::PluginLoadResult>>& cache() {
  static auto* libraries =
      new std::map<std::string, std::shared_ptr<oi::PluginLoadResult>>();
  return *libraries;
}

std::shared_ptr<oi::PluginLoadResult> loadCached(const QString& path) {
  const std::string key = toStd(path);
  QMutexLocker lock(&cacheMutex());
  auto it = cache().find(key);
  if (it != cache().end() && *it->second) {
    return it->second;
  }
  auto loaded =
      std::make_shared<oi::PluginLoadResult>(oi::PluginLoader::load(key));
  if (*loaded) {
    cache()[key] = loaded;
  }
  return loaded;
}

}  // namespace

oi::Plugin* PluginHost::plugin(const QString& path, QString* error) {
  auto loaded = loadCached(path);
  if (!*loaded) {
    if (error) {
      *error = toQString(loaded->error);
    }
    return nullptr;
  }
  return loaded->plugin.get();
}

PluginProbe PluginHost::probe(const QString& path) {
  PluginProbe result;
  auto loaded = loadCached(path);
  result.status = loaded->status;
  result.error = toQString(loaded->error);

  if (*loaded) {
    result.plugin = loaded->plugin.get();
    result.iid = Plugin_iidd;
    const std::string& json = loaded->plugin.metadataJson();
    const QJsonDocument doc = QJsonDocument::fromJson(
        QByteArray(json.data(), static_cast<qsizetype>(json.size())));
    result.meta = doc.object();
    result.isOiPlugin = result.meta.value("isOiPlugin").toBool();
    return result;
  }

  if (loaded->status == oi::PluginLoadStatus::kMissingEntryPoint ||
      loaded->status == oi::PluginLoadStatus::kCannotLoadLibrary) {
    // not a compute plugin: perhaps a library with UI tools only
    QPluginLoader qtProbe(path);
    const QJsonObject qtMeta = qtProbe.metaData();
    const QJsonObject meta = qtMeta.value("MetaData").toObject();
    if (meta.value("isOiPlugin").toBool()) {
      result.meta = meta;
      result.isOiPlugin = true;
      result.iid = qtMeta.value("IID").toString();
    }
  }
  return result;
}

bool PluginHost::readMetaData(const QString& path,
                              oi::PluginMetaData& metaData) {
  const PluginProbe probed = probe(path);
  if (!probed.isOiPlugin) {
    return false;
  }
  const QJsonObject& meta = probed.meta;
  metaData.iid = toStd(probed.iid);
  metaData.name = toStd(meta.value("name").toString());
  metaData.pluginVersion = toStd(meta.value("pluginVersion").toString());
  metaData.author = toStd(meta.value("author").toString());
  metaData.compiler = toStd(meta.value("compiler").toString());
  metaData.operatingSystem = toStd(meta.value("operatingSystem").toString());
  metaData.dependencies = meta.value("dependencies").toBool();
  metaData.description = toStd(meta.value("description").toString());
  metaData.dependenciesPath = toDependencies(meta.value("libPaths").toArray());
  metaData.path = toStd(path);
  return true;
}

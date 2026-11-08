#ifndef PLUGINHOST_H
#define PLUGINHOST_H

#include <QJsonObject>
#include <QString>

#include "plugin.h"
#include "plugin_loader.h"
#include "plugin_metadata.h"

/*!
 * \brief What the application learned about a plugin library.
 */
struct PluginProbe {
  oi::PluginLoadStatus status = oi::PluginLoadStatus::kCannotLoadLibrary;
  QString error;
  oi::Plugin* plugin =
      nullptr;  ///< compute factory; null for UI-tool-only libraries
  bool isOiPlugin = false;  ///< the library carries OpenIndy metadata
  QJsonObject meta;         ///< the plugin's meta_info.json content
  QString iid;              ///< interface id recorded in the plugin database
};

/*!
 * \brief Application-side host of plugin libraries.
 *
 * Compute plugins (sensors, functions, ...) are loaded with core's
 * oi::PluginLoader. A library stays loaded for the rest of the process:
 * sensors and functions created from it live in the model and in worker
 * threads, so unloading it would leave dangling vtables. Libraries that carry
 * UI tools (the Qt-side oi::ToolPlugin interface) are still opened with
 * QPluginLoader (see PluginManager::loadToolPlugins); a library may offer both.
 */
class PluginHost {
 public:
  /// Loads (once) and returns the compute factory of the library at path, or
  /// null with the reason in *error.
  static oi::Plugin* plugin(const QString& path, QString* error = nullptr);

  /// Loads the library and reads its metadata. Compute plugins are read from
  /// the metadata entry point; a library that is not a compute plugin but has
  /// Qt plugin metadata flagged isOiPlugin is reported as a UI-tool plugin.
  static PluginProbe probe(const QString& path);

  /// Fills metaData from the library at path. False if it is not an OpenIndy
  /// plugin (metaData is left untouched).
  static bool readMetaData(const QString& path, oi::PluginMetaData& metaData);
};

#endif  // PLUGINHOST_H

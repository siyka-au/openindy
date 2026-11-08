#ifndef OI_APP_CONTROLLER_PLUGIN_COPIER_H_
#define OI_APP_CONTROLLER_PLUGIN_COPIER_H_

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonObject>
#include <QObject>
#include <QPluginLoader>
#include <QString>
#include <QStringList>

#include "oi_meta_data.h"
#include "plugin_metadata.h"

using namespace oi;

class PluginManager;

class PluginCopier : public QObject {
  friend class PluginManager;
  Q_OBJECT

 public:
  explicit PluginCopier(QObject* parent = nullptr);

 public slots:

  // ########################
  // import or remove plugins
  // ########################

  void importPlugin(const QString& path);
  void deletePlugin(const QString& name);

 signals:

  // #####################
  // inform about progress
  // #####################

  void updateProgress(const int& progress);
  void sendMessage(const QString& msg, const MessageTypes& msgType,
                   const MessageDestinations& msgDest =
                       MessageDestinations::eConsoleMessage);
  void importFinished(const bool& success);
  void deletionFinished(const bool& success);

 private:
  // ##############
  // helper methods
  // ##############

  // reads a candidate plugin's own embedded Qt metadata (name, author,
  // dependencies, ...) - no compatibility gate here, Qt's own plugin
  // loader already enforces ABI compatibility when the app later loads it
  void readPluginMetaData(PluginMetaData& metaData, const QString& path);

  bool copyDir(const QString& sourcePath, const QString& destinationPath);
  bool checkDependencies(const QString& sourcePath,
                         const PluginMetaData& metaData);
};

#endif  // OI_APP_CONTROLLER_PLUGIN_COPIER_H_

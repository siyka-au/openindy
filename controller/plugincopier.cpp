#include "plugincopier.h"

#include "pluginloader.h"

/*!
 * \brief PluginCopier::PluginCopier
 * \param parent
 */
PluginCopier::PluginCopier(QObject* parent) : QObject(parent) {}

/*!
 * \brief PluginCopier::importPlugin
 * Copies the plugin (and any declared dependencies) into the user tier,
 * ~/.openindy/plugins/ - the only location a regular user can write to
 * without elevated permissions. PluginRepository picks it up on its next
 * scan (app restart), no further registration step needed.
 * \param path
 */
void PluginCopier::importPlugin(const QString& path) {
  // read the candidate plugin's own metadata (dependencies list etc.)
  PluginMetaData metaData;
  this->readPluginMetaData(metaData, path);

  // user tier root
  QDir userDir(QDir::homePath() + "/.openindy");

  // get file info and check if plugin already exists
  QFileInfo pluginFileInfo(path);
  QDir pluginDir(userDir.absolutePath() + "/plugins");
  pluginDir.mkpath(pluginDir.absolutePath());

  if (QFile::exists(pluginDir.absoluteFilePath(pluginFileInfo.fileName()))) {
    emit this->sendMessage(
        QString("Plugin %1 already exists").arg(pluginFileInfo.fileName()),
        eErrorMessage);
    emit this->importFinished(false);
    return;
  }

  // copy dependencies
  if (metaData.dependencies) {
    // check if the dependencies exist
    if (!this->checkDependencies(pluginFileInfo.absoluteDir().absolutePath(),
                                 metaData)) {
      emit this->sendMessage(
          QString("Not all specified dependencies could be found for plugin %1")
              .arg(pluginFileInfo.fileName()),
          eCriticalMessage);
      emit this->importFinished(false);
      return;
    }

    // copy all dependencies
    for (int i = 0; i < metaData.dependenciesPath.size(); i++) {
      // get source path
      QString sourcePath(
          pluginFileInfo.absoluteDir().absolutePath() + "/" +
          metaData.dependenciesPath.at(i).toObject().value("name").toString());
      QFileInfo sourcePathInfo(sourcePath);

      // get destination path
      QString destPath;
      if (metaData.dependenciesPath.at(i)
              .toObject()
              .value("path")
              .toString()
              .compare("app") == 0) {
        destPath = QString(userDir.absolutePath() + "/" +
                           metaData.dependenciesPath.at(i)
                               .toObject()
                               .value("name")
                               .toString());
      } else {
        destPath = QString(metaData.dependenciesPath.at(i)
                               .toObject()
                               .value("path")
                               .toString() +
                           "/" +
                           metaData.dependenciesPath.at(i)
                               .toObject()
                               .value("name")
                               .toString());
      }

      // copy from source to destination
      if (sourcePathInfo.isFile()) {
        if (QFileInfo::exists(destPath)) {
          QFile::remove(destPath);
        }
        QFile::copy(sourcePath, destPath);

      } else if (sourcePathInfo.isDir()) {
        this->copyDir(sourcePath, destPath);

      } else {
        emit this->sendMessage(
            QString("Copy failed for file %1 (unknown file format)")
                .arg(sourcePathInfo.fileName()),
            eCriticalMessage);
      }

      emit this->updateProgress((i * 100) / metaData.dependenciesPath.size());
    }
  }

  // copy plugin itself
  if (!QFile::copy(pluginFileInfo.absoluteFilePath(),
                   pluginDir.absoluteFilePath(pluginFileInfo.fileName()))) {
    emit this->sendMessage(
        QString("Copy failed for plugin %1").arg(pluginFileInfo.fileName()),
        eErrorMessage);
    emit this->importFinished(false);
    return;
  }

  emit this->importFinished(true);
}

/*!
 * \brief PluginCopier::deletePlugin
 * \param name
 */
void PluginCopier::deletePlugin(const QString& name) {
  // TODO delete plugins

  emit this->deletionFinished(false);
}

/*!
 * \brief PluginCopier::readPluginMetaData
 * \param metaData
 * \param path
 */
void PluginCopier::readPluginMetaData(PluginMetaData& metaData,
                                      const QString& path) {
  QPluginLoader pluginLoader(path);
  metaData.iid = pluginLoader.metaData().value("IID").toString();
  metaData.name = pluginLoader.metaData()
                      .value("MetaData")
                      .toObject()
                      .value("name")
                      .toString();
  metaData.pluginVersion = pluginLoader.metaData()
                               .value("MetaData")
                               .toObject()
                               .value("pluginVersion")
                               .toString();
  metaData.author = pluginLoader.metaData()
                        .value("MetaData")
                        .toObject()
                        .value("author")
                        .toString();
  metaData.compiler = pluginLoader.metaData()
                          .value("MetaData")
                          .toObject()
                          .value("compiler")
                          .toString();
  metaData.operatingSystem = pluginLoader.metaData()
                                 .value("MetaData")
                                 .toObject()
                                 .value("operatingSystem")
                                 .toString();
  metaData.dependencies = pluginLoader.metaData()
                              .value("MetaData")
                              .toObject()
                              .value("dependencies")
                              .toBool();
  metaData.description = pluginLoader.metaData()
                             .value("MetaData")
                             .toObject()
                             .value("description")
                             .toString();
  metaData.dependenciesPath = pluginLoader.metaData()
                                  .value("MetaData")
                                  .toObject()
                                  .value("libPaths")
                                  .toArray();
}

/*!
 * \brief PluginCopier::copyDir
 * Copies all files and folders of sourcePath to destinationPath
 * \param sourcePath
 * \param destinationPath
 * \return
 */
bool PluginCopier::copyDir(const QString& sourcePath,
                           const QString& destinationPath) {
  QDir destDir;
  destDir.mkpath(destinationPath);
  QDir sourceDir(sourcePath);

  // get lists of files and folders in source folder
  QStringList destFiles =
      sourceDir.entryList(QDir::NoDotAndDotDot | QDir::Files);
  QStringList destDirs = sourceDir.entryList(QDir::NoDotAndDotDot | QDir::Dirs);

  // remove files and folders in destination folder and add source files and
  // folders
  for (const auto& destFile : destFiles) {
    if (QFileInfo::exists(destinationPath + "/" + destFile)) {
      QFile::remove(destinationPath + "/" + destFile);
    }

    if (!QFile::copy(sourcePath + "/" + destFile,
                     destinationPath + "/" + destFile)) {
      return false;
    } else {
    }
  }
  foreach (const QString& dir, destDirs) {
    this->copyDir(sourcePath + "/" + dir, destinationPath + "/" + dir);
  }

  return true;
}

/*!
 * \brief PluginCopier::checkDependencies
 * Checks wether the dependencies at sourcePath exist or not
 * \param sourcePath
 * \param metaData
 * \return
 */
bool PluginCopier::checkDependencies(const QString& sourcePath,
                                     const PluginMetaData& metaData) {
  for (int i = 0; i < metaData.dependenciesPath.size(); i++) {
    // get source file path
    QString p(
        sourcePath + "/" +
        metaData.dependenciesPath.at(i).toObject().value("name").toString());
    QFileInfo fileInfo(p);

    if (!fileInfo.exists()) {
      emit this->sendMessage(
          QString("dependency %1 does not exist").arg(fileInfo.fileName()),
          eCriticalMessage);
      return false;
    }
  }

  return true;
}

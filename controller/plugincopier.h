#ifndef PLUGINCOPIER_H
#define PLUGINCOPIER_H

#include <QObject>
#include <QDir>
#include <QApplication>
#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QFileInfo>
#include <QPluginLoader>

#include "pluginmetadata.h"
#include "oimetadata.h"

using namespace oi;

class PluginLoader;

class PluginCopier : public QObject
{
    friend class PluginLoader;
    Q_OBJECT

public:
    explicit PluginCopier(QObject *parent = 0);

public slots:

    //########################
    //import or remove plugins
    //########################

    void importPlugin(const QString &path);
    void deletePlugin(const QString &name);

signals:

    //#####################
    //inform about progress
    //#####################

    void updateProgress(const int &progress);
    void sendMessage(const QString &msg, const MessageTypes &msgType, const MessageDestinations &msgDest = eConsoleMessage);
    void importFinished(const bool &success);
    void deletionFinished(const bool &success);

private:

    //##############
    //helper methods
    //##############

    //reads a candidate plugin's own embedded Qt metadata (name, author,
    //dependencies, ...) - no compatibility gate here, Qt's own plugin
    //loader already enforces ABI compatibility when the app later loads it
    void readPluginMetaData(PluginMetaData &metaData, const QString &path);

    bool copyDir(const QString &sourcePath, const QString &destinationPath);
    bool checkDependencies(const QString &sourcePath, const PluginMetaData &metaData);

};

#endif // PLUGINCOPIER_H

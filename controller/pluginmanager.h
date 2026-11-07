#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

#include <QApplication>
#include <QDir>
#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QObject>
#include <QPluginLoader>
#include <QPointer>
#include <QString>
#include <memory>
#include <vector>

#include "bundleadjustment.h"
#include "console.h"
#include "exchangedefinedformat.h"
#include "exchangeinterface.h"
#include "exchangesimpleascii.h"
#include "function.h"
#include "plugin.h"
#include "plugincopier.h"
#include "pluginmetadata.h"
#include "sensor.h"
#include "simulationmodel.h"
#include "tool.h"

using namespace oi;

/*!
 * \brief The PluginManager class
 * Loads OpenIndy plugins
 */
class PluginManager : public QObject {
  Q_OBJECT

 private:
  explicit PluginManager(QObject* parent = nullptr);

 public:
  // ##########################
  // get plugin loader instance
  // ##########################

  static QPointer<PluginManager> getInstance();

  // #################################
  // get plugin meta data as a preview
  // #################################

  static PluginMetaData getPluginMetaData(const QString& path);

  // ########################
  // import or remove plugins
  // ########################

  static void importPlugin(const QString& path);
  static void deletePlugin(const QString& name);

  // #####################
  // load OpenIndy plugins
  // #####################

  static std::unique_ptr<Sensor> loadSensorPlugin(const QString& path,
                                                  const QString& name);
  static std::unique_ptr<Function> loadFunctionPlugin(const QString& path,
                                                      const QString& name);
  static std::unique_ptr<SimulationModel> loadSimulationPlugin(
      const QString& path, const QString& name);
  static std::unique_ptr<BundleAdjustment> loadBundleAdjustmentPlugin(
      const QString& path, const QString& name);
  static QPointer<Tool> loadToolPlugin(const QString& path,
                                       const QString& name);
  static std::unique_ptr<ExchangeSimpleAscii> loadExchangeSimpleAsciiPlugin(
      const QString& path, const QString& name);
  static std::unique_ptr<ExchangeDefinedFormat> loadExchangeDefinedFormatPlugin(
      const QString& path, const QString& name);

  static std::vector<std::unique_ptr<Sensor> > loadSensorPlugins(
      const QString& path);
  static std::vector<std::unique_ptr<Function> > loadFunctionPlugins(
      const QString& path);
  static std::vector<std::unique_ptr<SimulationModel> > loadSimulationPlugins(
      const QString& path);
  static std::vector<std::unique_ptr<BundleAdjustment> >
  loadBundleAdjustmentPlugins(const QString& path);
  static QList<QPointer<Tool> > loadToolPlugins(const QString& path);
  static std::vector<std::unique_ptr<ExchangeSimpleAscii> >
  loadExchangeSimpleAsciiPlugins(const QString& path);
  static std::vector<std::unique_ptr<ExchangeDefinedFormat> >
  loadExchangeDefinedFormatPlugins(const QString& path);

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

  // ############
  // start import
  // ############

  void startImport(const QString& path);
  void startDeletion(const QString& name);

 private slots:

  // ###############
  // clean up copier
  // ###############

  void importTaskFinished(const bool& success);

 private:
  // #############################
  // single plugin loader instance
  // #############################

  static QPointer<PluginManager> myInstance;

  // ########################
  // plugin copier and thread
  // ########################

  static PluginCopier pluginCopier;
  static QThread copierThread;
};

#endif  // PLUGINMANAGER_H

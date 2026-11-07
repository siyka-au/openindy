#ifndef PLUGINLOADER_H
#define PLUGINLOADER_H

#include <QApplication>
#include <QDir>
#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QObject>
#include <QPluginLoader>
#include <QPointer>
#include <QString>
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
 * \brief The PluginLoader class
 * Loads OpenIndy plugins
 */
class PluginLoader : public QObject {
  Q_OBJECT

 private:
  explicit PluginLoader(QObject* parent = nullptr);

 public:
  // ##########################
  // get plugin loader instance
  // ##########################

  static QPointer<PluginLoader> getInstance();

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

  static oi::Guarded<Sensor> loadSensorPlugin(const QString& path,
                                              const QString& name);
  static oi::Guarded<Function> loadFunctionPlugin(const QString& path,
                                                  const QString& name);
  static oi::Guarded<SimulationModel> loadSimulationPlugin(const QString& path,
                                                           const QString& name);
  static oi::Guarded<BundleAdjustment> loadBundleAdjustmentPlugin(
      const QString& path, const QString& name);
  static QPointer<Tool> loadToolPlugin(const QString& path,
                                       const QString& name);
  static oi::Guarded<ExchangeSimpleAscii> loadExchangeSimpleAsciiPlugin(
      const QString& path, const QString& name);
  static oi::Guarded<ExchangeDefinedFormat> loadExchangeDefinedFormatPlugin(
      const QString& path, const QString& name);

  static std::vector<oi::Guarded<Sensor> > loadSensorPlugins(
      const QString& path);
  static std::vector<oi::Guarded<Function> > loadFunctionPlugins(
      const QString& path);
  static std::vector<oi::Guarded<SimulationModel> > loadSimulationPlugins(
      const QString& path);
  static std::vector<oi::Guarded<BundleAdjustment> >
  loadBundleAdjustmentPlugins(const QString& path);
  static QList<QPointer<Tool> > loadToolPlugins(const QString& path);
  static std::vector<oi::Guarded<ExchangeSimpleAscii> >
  loadExchangeSimpleAsciiPlugins(const QString& path);
  static std::vector<oi::Guarded<ExchangeDefinedFormat> >
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

  static QPointer<PluginLoader> myInstance;

  // ########################
  // plugin copier and thread
  // ########################

  static PluginCopier pluginCopier;
  static QThread copierThread;
};

#endif  // PLUGINLOADER_H

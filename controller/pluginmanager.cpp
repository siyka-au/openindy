#include "pluginmanager.h"

#include <QMutex>
#include <QMutexLocker>
#include <map>
#include <vector>

#include "pluginhost.h"
#include "qtconvert.h"

// initialize static variables
QPointer<PluginManager> PluginManager::myInstance(nullptr);
PluginCopier PluginManager::pluginCopier;
QThread PluginManager::copierThread;

/*!
 * \brief PluginManager::importPlugin
 * \param path
 */
PluginManager::PluginManager(QObject* parent) : QObject(parent) {
  // connect plugin copier
  QObject::connect(this, SIGNAL(startImport(const QString&)),
                   &PluginManager::pluginCopier,
                   SLOT(importPlugin(const QString&)), Qt::AutoConnection);
  QObject::connect(this, SIGNAL(startDeletion(const QString&)),
                   &PluginManager::pluginCopier,
                   SLOT(deletePlugin(const QString&)), Qt::AutoConnection);

  QObject::connect(&PluginManager::pluginCopier,
                   SIGNAL(updateProgress(const int&)), this,
                   SIGNAL(updateProgress(const int&)), Qt::AutoConnection);
  QObject::connect(&PluginManager::pluginCopier,
                   SIGNAL(sendMessage(const QString&, const MessageTypes&,
                                      const MessageDestinations&)),
                   this,
                   SIGNAL(sendMessage(const QString&, const MessageTypes&,
                                      const MessageDestinations&)),
                   Qt::AutoConnection);
  QObject::connect(&PluginManager::pluginCopier,
                   SIGNAL(deletionFinished(const bool&)), this,
                   SIGNAL(deletionFinished(const bool&)), Qt::AutoConnection);

  QObject::connect(&PluginManager::pluginCopier,
                   SIGNAL(importFinished(const bool&)), this,
                   SLOT(importTaskFinished(const bool&)), Qt::AutoConnection);
}

/*!
 * \brief PluginManager::getInstance
 * \return
 */
QPointer<PluginManager> PluginManager::getInstance() {
  if (PluginManager::myInstance.isNull()) {
    PluginManager::myInstance = new PluginManager();
  }
  return PluginManager::myInstance;
}

/*!
 * \brief PluginManager::getPluginMetaData
 * Get plugin meta data as a preview
 * \param path
 * \return
 */
PluginMetaData PluginManager::getPluginMetaData(const QString& path) {
  PluginMetaData metaData;

  // search in the given folder for a valid plugin
  QDir pluginsDir(path);

  foreach (const QString& fileName,
           pluginsDir.entryList(QStringList() << "p_*.dll" << "libp_*.so",
                                QDir::Files)) {
    if (PluginHost::readMetaData(pluginsDir.absoluteFilePath(fileName),
                                 metaData)) {
      break;
    }
  }

  return metaData;
}

/*!
 * \brief PluginManager::importPlugin
 * Imports the plugin at file path path
 * \param path
 */
void PluginManager::importPlugin(const QString& path) {
  QPointer<PluginManager> pluginLoader = PluginManager::getInstance();

  // move copier to thread
  PluginManager::pluginCopier.moveToThread(&PluginManager::copierThread);
  PluginManager::copierThread.start();

  emit pluginLoader->startImport(path);
}

/*!
 * \brief PluginManager::deletePlugin
 * Deletes the plugin with the name name
 * \param name
 */
void PluginManager::deletePlugin(const QString& name) {
  QPointer<PluginManager> pluginLoader = PluginManager::getInstance();

  // move copier to thread
  PluginManager::pluginCopier.moveToThread(&PluginManager::copierThread);
  PluginManager::copierThread.start();

  emit pluginLoader->startDeletion(name);
}

/*!
 * \brief PluginManager::reportLoadError
 * Sends a console message when the plugin library could not be loaded
 */
static void reportLoadError(const QString& what, const QString& error) {
  emit PluginManager::getInstance() -> sendMessage(
      QString("Cannot load %1: %2").arg(what).arg(error),
      MessageTypes::eErrorMessage, MessageDestinations::eConsoleMessage);
}

/*!
 * \brief toolPluginOf
 * UI tools are a Qt plugin interface, so a library offering them is opened
 * with QPluginLoader and the interface obtained by qobject_cast. A library
 * without Qt plugin metadata (a compute plugin) simply has no tools. The
 * QPluginLoader keeps the library loaded for the process lifetime.
 */
static ToolPlugin* toolPluginOf(const QString& path) {
  static QMutex mutex;
  static std::map<QString, QPluginLoader*> loaders;  // never unloaded
  QMutexLocker lock(&mutex);
  auto it = loaders.find(path);
  if (it == loaders.end()) {
    it = loaders.emplace(path, new QPluginLoader(path)).first;
  }
  QPluginLoader* pluginLoader = it->second;
  if (pluginLoader->metaData().isEmpty()) {
    return nullptr;
  }
  QObject* plugin = pluginLoader->instance();
  if (!plugin) {
    reportLoadError("tools", pluginLoader->errorString());
    return nullptr;
  }
  return qobject_cast<ToolPlugin*>(plugin);
}

/*!
 * \brief PluginManager::loadSensorPlugin
 * \param path
 * \param name
 * \return
 */
oi::Guarded<Sensor> PluginManager::loadSensorPlugin(const QString& path,
                                                    const QString& name) {
  oi::Guarded<Sensor> sensor(nullptr);

  QString error;
  Plugin* factory = PluginHost::plugin(path, &error);
  if (factory) {
    sensor = factory->createSensor(toStd(name));
    if (!sensor.isNull()) {
      sensor->init();
    }
  } else {
    reportLoadError(QString("selected sensor: %1").arg(name), error);
  }

  return sensor;
}

/*!
 * \brief PluginManager::loadFunctionPlugin
 * \param path
 * \param name
 * \return
 */
oi::Guarded<Function> PluginManager::loadFunctionPlugin(const QString& path,
                                                        const QString& name) {
  oi::Guarded<Function> function(nullptr);

  QString error;
  Plugin* factory = PluginHost::plugin(path, &error);
  if (factory) {
    function = factory->createFunction(toStd(name));
    if (!function.isNull()) {
      function->init();
    }
  } else {
    reportLoadError(QString("selected function: %1").arg(name), error);
  }

  return function;
}

/*!
 * \brief PluginManager::loadSimulationPlugin
 * \param path
 * \param name
 * \return
 */
oi::Guarded<SimulationModel> PluginManager::loadSimulationPlugin(
    const QString& path, const QString& name) {
  oi::Guarded<SimulationModel> simulation(nullptr);

  QString error;
  Plugin* factory = PluginHost::plugin(path, &error);
  if (factory) {
    simulation = factory->createSimulation(toStd(name));
    if (!simulation.isNull()) {
      simulation->init();
    }
  } else {
    reportLoadError(QString("selected simulation: %1").arg(name), error);
  }

  return simulation;
}

/*!
 * \brief PluginManager::loadBundleAdjustmentPlugin
 * \param path
 * \param name
 * \return
 */
oi::Guarded<BundleAdjustment> PluginManager::loadBundleAdjustmentPlugin(
    const QString& path, const QString& name) {
  oi::Guarded<BundleAdjustment> bundleAdjustment(nullptr);

  QString error;
  Plugin* factory = PluginHost::plugin(path, &error);
  if (factory) {
    bundleAdjustment = factory->createBundleAdjustment(toStd(name));
    if (!bundleAdjustment.isNull()) {
      bundleAdjustment->init();
    }
  } else {
    reportLoadError(QString("selected bundle adjustment: %1").arg(name), error);
  }

  return bundleAdjustment;
}

/*!
 * \brief PluginManager::loadExchangeSimpleAsciiPlugin
 * \param path
 * \param name
 * \return
 */
oi::Guarded<ExchangeSimpleAscii> PluginManager::loadExchangeSimpleAsciiPlugin(
    const QString& path, const QString& name) {
  oi::Guarded<ExchangeSimpleAscii> exchangeSimpleAscii(nullptr);

  QString error;
  Plugin* factory = PluginHost::plugin(path, &error);
  if (factory) {
    exchangeSimpleAscii = factory->createSimpleAsciiExchange(toStd(name));
    if (!exchangeSimpleAscii.isNull()) {
      exchangeSimpleAscii->init();
    }
  } else {
    reportLoadError(QString("selected simple ascii exchange: %1").arg(name),
                    error);
  }

  return exchangeSimpleAscii;
}

/*!
 * \brief PluginManager::loadExchangeDefinedFormatPlugin
 * \param path
 * \param name
 * \return
 */
oi::Guarded<ExchangeDefinedFormat>
PluginManager::loadExchangeDefinedFormatPlugin(const QString& path,
                                               const QString& name) {
  oi::Guarded<ExchangeDefinedFormat> exchangeDefinedFormat(nullptr);

  QString error;
  Plugin* factory = PluginHost::plugin(path, &error);
  if (factory) {
    exchangeDefinedFormat = factory->createDefinedFormatExchange(toStd(name));
    if (!exchangeDefinedFormat.isNull()) {
      exchangeDefinedFormat->init();
    }
  } else {
    reportLoadError(QString("selected defined format exchange: %1").arg(name),
                    error);
  }

  return exchangeDefinedFormat;
}

/*!
 * \brief PluginManager::loadSensorPlugins
 * \param path
 * \return
 */
std::vector<oi::Guarded<Sensor> > PluginManager::loadSensorPlugins(
    const QString& path) {
  std::vector<oi::Guarded<Sensor> > sensorList;

  QString error;
  Plugin* factory = PluginHost::plugin(path, &error);
  if (factory) {
    sensorList = factory->createSensors();
    for (const oi::Guarded<Sensor>& item : sensorList) {
      if (!item.isNull()) {
        item->init();
      }
    }
  } else {
    reportLoadError("sensors", error);
  }

  return sensorList;
}

/*!
 * \brief PluginManager::loadFunctionPlugins
 * \param path
 * \return
 */
std::vector<oi::Guarded<Function> > PluginManager::loadFunctionPlugins(
    const QString& path) {
  std::vector<oi::Guarded<Function> > functionList;

  QString error;
  Plugin* factory = PluginHost::plugin(path, &error);
  if (factory) {
    functionList = factory->createFunctions();
    for (const oi::Guarded<Function>& item : functionList) {
      if (!item.isNull()) {
        item->init();
      }
    }
  } else {
    reportLoadError("functions", error);
  }

  return functionList;
}

/*!
 * \brief PluginManager::loadSimulationPlugins
 * \param path
 * \return
 */
std::vector<oi::Guarded<SimulationModel> > PluginManager::loadSimulationPlugins(
    const QString& path) {
  std::vector<oi::Guarded<SimulationModel> > simulationList;

  QString error;
  Plugin* factory = PluginHost::plugin(path, &error);
  if (factory) {
    simulationList = factory->createSimulations();
    for (const oi::Guarded<SimulationModel>& item : simulationList) {
      if (!item.isNull()) {
        item->init();
      }
    }
  } else {
    reportLoadError("simulations", error);
  }

  return simulationList;
}

/*!
 * \brief PluginManager::loadBundleAdjustmentPlugins
 * \param path
 * \return
 */
std::vector<oi::Guarded<BundleAdjustment> >
PluginManager::loadBundleAdjustmentPlugins(const QString& path) {
  std::vector<oi::Guarded<BundleAdjustment> > bundleAdjustmentList;

  QString error;
  Plugin* factory = PluginHost::plugin(path, &error);
  if (factory) {
    bundleAdjustmentList = factory->createBundleAdjustments();
    for (const oi::Guarded<BundleAdjustment>& item : bundleAdjustmentList) {
      if (!item.isNull()) {
        item->init();
      }
    }
  } else {
    reportLoadError("bundle adjustments", error);
  }

  return bundleAdjustmentList;
}

/*!
 * \brief PluginManager::loadExchangeSimpleAsciiPlugins
 * \param path
 * \return
 */
std::vector<oi::Guarded<ExchangeSimpleAscii> >
PluginManager::loadExchangeSimpleAsciiPlugins(const QString& path) {
  std::vector<oi::Guarded<ExchangeSimpleAscii> > exchangeSimpleAsciiList;

  QString error;
  Plugin* factory = PluginHost::plugin(path, &error);
  if (factory) {
    exchangeSimpleAsciiList = factory->createSimpleAsciiExchanges();
    for (const oi::Guarded<ExchangeSimpleAscii>& item :
         exchangeSimpleAsciiList) {
      if (!item.isNull()) {
        item->init();
      }
    }
  } else {
    reportLoadError("simple ascii exchanges", error);
  }

  return exchangeSimpleAsciiList;
}

/*!
 * \brief PluginManager::loadExchangeDefinedFormatPlugins
 * \param path
 * \return
 */
std::vector<oi::Guarded<ExchangeDefinedFormat> >
PluginManager::loadExchangeDefinedFormatPlugins(const QString& path) {
  std::vector<oi::Guarded<ExchangeDefinedFormat> > exchangeDefinedFormatList;

  QString error;
  Plugin* factory = PluginHost::plugin(path, &error);
  if (factory) {
    exchangeDefinedFormatList = factory->createDefinedFormatExchanges();
    for (const oi::Guarded<ExchangeDefinedFormat>& item :
         exchangeDefinedFormatList) {
      if (!item.isNull()) {
        item->init();
      }
    }
  } else {
    reportLoadError("defined format exchanges", error);
  }

  return exchangeDefinedFormatList;
}

/*!
 * \brief PluginManager::loadToolPlugin
 * \param path
 * \param name
 * \return
 */
QPointer<Tool> PluginManager::loadToolPlugin(const QString& path,
                                             const QString& name) {
  QPointer<Tool> tool(nullptr);

  ToolPlugin* toolFactory = toolPluginOf(path);
  if (toolFactory) {
    tool = toolFactory->createTool(name);
    if (!tool.isNull()) {
      tool->init();
    }
  }

  return tool;
}

/*!
 * \brief PluginManager::loadToolPlugins
 * \param path
 * \return
 */
QList<QPointer<Tool> > PluginManager::loadToolPlugins(const QString& path) {
  QList<QPointer<Tool> > toolList;

  ToolPlugin* toolFactory = toolPluginOf(path);
  if (toolFactory) {
    toolList = toolFactory->createTools();
    for (const QPointer<Tool>& tool : toolList) {
      if (!tool.isNull()) {
        tool->init();
      }
    }
  }

  return toolList;
}

/*!
 * \brief PluginManager::importTaskFinished
 * Is called when the plugin copier has finished a task (import or deletion)
 * \param success
 */
void PluginManager::importTaskFinished(const bool& success) {
  PluginManager::copierThread.quit();
  PluginManager::copierThread.wait();

  // PluginCopier::importPlugin() already did all the work (copy the file
  // and any dependencies into ~/.openindy/plugins/) - PluginRepository
  // picks it up on its next scan, no separate registration step needed
  emit this->importFinished(success);
}

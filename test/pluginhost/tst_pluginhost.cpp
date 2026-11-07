#include <QCoreApplication>
#include <QFile>
#include <QtTest>

#include "chooselalib.h"
#include "pluginhost.h"
#include "pluginmanager.h"
#include "pluginrepository.h"

/*!
 * The application loads compute plugins with core's oi::PluginLoader through
 * PluginHost, and reads their metadata from the metadata entry point. This
 * loads the two plugins built next to the application the way the app does at
 * startup.
 */
class PluginHostTest : public QObject {
  Q_OBJECT

 private slots:
  void initTestCase();
  void probesDefaultPlugin();
  void probesLeicaPlugin();
  void readsMetaDataForTheCopier();
  void loadsFunctionsAndSensors();
  void notAPluginIsNotAPlugin();
  void libraryWithoutToolsHasNoTools();
  void scansADirectory();
};

void PluginHostTest::initTestCase() {
  ChooseLALib::setLinearAlgebra(ChooseLALib::LinearAlgebraLibrary::Armadillo);
}

void PluginHostTest::probesDefaultPlugin() {
  const PluginProbe probe = PluginHost::probe(DEFAULT_PLUGIN_PATH);
  QVERIFY2(probe.status == oi::PluginLoadStatus::kOk, qPrintable(probe.error));
  QVERIFY(probe.plugin != nullptr);
  QVERIFY(probe.isOiPlugin);
  QCOMPARE(probe.meta.value("abiVersion").toInt(), OI_PLUGIN_ABI_VERSION);
  QVERIFY(!probe.meta.value("name").toString().isEmpty());
  QVERIFY(!probe.meta.value("compiler").toString().isEmpty());
  QVERIFY(!probe.meta.value("operatingSystem").toString().isEmpty());
  QCOMPARE(probe.iid, QString(Plugin_iidd));
}

void PluginHostTest::probesLeicaPlugin() {
  const PluginProbe probe = PluginHost::probe(LEICA_PLUGIN_PATH);
  QVERIFY2(probe.isOiPlugin, qPrintable(probe.error));
  QCOMPARE(probe.meta.value("abiVersion").toInt(), OI_PLUGIN_ABI_VERSION);
}

void PluginHostTest::readsMetaDataForTheCopier() {
  oi::PluginMetaData metaData;
  QVERIFY(PluginHost::readMetaData(DEFAULT_PLUGIN_PATH, metaData));
  QVERIFY(!metaData.name.empty());
  QVERIFY(!metaData.compiler.empty());
  QVERIFY(!metaData.operatingSystem.empty());
  QCOMPARE(metaData.path, std::string(DEFAULT_PLUGIN_PATH));
}

void PluginHostTest::loadsFunctionsAndSensors() {
  const QString path = DEFAULT_PLUGIN_PATH;
  QVERIFY(!PluginManager::loadFunctionPlugins(path).empty());
  QVERIFY(!PluginManager::loadSensorPlugins(path).empty());
  QVERIFY(!PluginManager::loadSensorPlugin(path, "PseudoTracker").isNull());
  // the same library again comes from the process-wide cache
  QCOMPARE(PluginHost::plugin(path), PluginHost::plugin(path));
}

void PluginHostTest::notAPluginIsNotAPlugin() {
  oi::PluginMetaData metaData;
  QVERIFY(!PluginHost::readMetaData(NOT_A_PLUGIN_PATH, metaData));
  QVERIFY(!PluginHost::readMetaData("/nonexistent/libp_nothing.so", metaData));
  QString error;
  QVERIFY(PluginHost::plugin("/nonexistent/libp_nothing.so", &error) ==
          nullptr);
  QVERIFY(!error.isEmpty());
}

void PluginHostTest::libraryWithoutToolsHasNoTools() {
  QVERIFY(PluginManager::loadToolPlugins(DEFAULT_PLUGIN_PATH).isEmpty());
  QVERIFY(PluginManager::loadToolPlugin(DEFAULT_PLUGIN_PATH, "x").isNull());
}

void PluginHostTest::scansADirectory() {
  const auto plugins = PluginRepository::scanDirectory(PLUGIN_DIR);
  QVERIFY(plugins.size() >= 2);
  bool hasFunctions = false;
  for (const auto& plugin : plugins) {
    hasFunctions = hasFunctions || !plugin.functions().empty();
  }
  QVERIFY(hasFunctions);
  QVERIFY(PluginRepository::getRejectedPlugins().isEmpty());
}

QTEST_MAIN(PluginHostTest)
#include "tst_pluginhost.moc"

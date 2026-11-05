#ifndef PLUGININFO_H
#define PLUGININFO_H

#include <QList>
#include <QString>

#include "measurementconfig.h"
#include "types.h"

#if defined(OI_MAIN_LIB)
#define OI_MAIN_EXPORT Q_DECL_EXPORT
#else
#define OI_MAIN_EXPORT Q_DECL_IMPORT
#endif

using namespace oi;

// ############################################################
// domain objects describing what a plugin offers, independent
// of how that information was discovered (directory scan today,
// previously a SQLite catalog) - consumers only ever see these.
// Immutable value objects: built once by PluginRepository's scan
// via the full constructor, read afterwards only through the
// const accessors below - no public fields, no setters.
// ############################################################

namespace sys {

class Function;
class Sensor;
class Tool;
class Exchange;
class Simulation;

class OI_MAIN_EXPORT Plugin {
 public:
  Plugin() {}
  Plugin(const QString& name, const QString& description,
         const QString& version, const QString& author, const QString& filePath,
         const QList<Function>& functions, const QList<Sensor>& sensors,
         const QList<Tool>& tools, const QList<Exchange>& exchanges,
         const QList<Simulation>& simulations)
      : m_name(name),
        m_description(description),
        m_version(version),
        m_author(author),
        m_filePath(filePath),
        m_functions(functions),
        m_sensors(sensors),
        m_tools(tools),
        m_exchanges(exchanges),
        m_simulations(simulations) {}

  QString name() const { return m_name; }
  QString description() const { return m_description; }
  QString version() const { return m_version; }
  QString author() const { return m_author; }
  QString filePath() const {
    return m_filePath;
  }  // the real, collision-safe identity - see PluginRepository

  QList<Function> functions() const { return m_functions; }
  QList<Sensor> sensors() const { return m_sensors; }
  QList<Tool> tools() const { return m_tools; }
  QList<Exchange> exchanges() const { return m_exchanges; }
  QList<Simulation> simulations() const { return m_simulations; }

 private:
  QString m_name;
  QString m_description;
  QString m_version;
  QString m_author;
  QString m_filePath;

  QList<Function> m_functions;
  QList<Sensor> m_sensors;
  QList<Tool> m_tools;
  QList<Exchange> m_exchanges;
  QList<Simulation> m_simulations;
};

//! shared shape: a named item offered by a plugin, identified within that
//! plugin by name alone (unique within one plugin's own class list, not
//! globally - see PluginRepository)
class OI_MAIN_EXPORT Sensor {
 public:
  Sensor() {}
  Sensor(
      const QString& name, const QString& description, const Plugin& plugin,
      const QString& interfaceCategory,
      const QList<MeasurementConfig>& measurementConfigs =
          QList<MeasurementConfig>(),
      const QList<AcquisitionMode>& acquisitionModes = QList<AcquisitionMode>())
      : m_name(name),
        m_description(description),
        m_plugin(plugin),
        m_interfaceCategory(interfaceCategory),
        m_measurementConfigs(measurementConfigs),
        m_acquisitionModes(acquisitionModes) {}

  QString name() const { return m_name; }
  QString description() const { return m_description; }
  Plugin plugin() const {
    return m_plugin;
  }  // owning plugin, so callers can always disambiguate
  QString qualifiedId() const {
    return m_plugin.filePath() + "#" + m_name;
  }  // real, collision-safe identity

  //! Sensor_iidd/LaserTracker_iidd/TotalStation_iidd - NOT an identity (shared
  //! by every sensor of that category), kept only to classify sensor type the
  //! same way OiMetaData::getSensorType() derives it from a plugin's own iid
  QString interfaceCategory() const { return m_interfaceCategory; }

  //! the config catalog and acquisition capability this sensor's own init()
  //! declared, captured once at scan time (the same instantiate+init() call
  //! that already builds this sys::Sensor at all) - MeasurementConfigManager
  //! merges these across every scanned sensor, not just a connected one, so
  //! the picker has real options before anything is plugged in
  QList<MeasurementConfig> measurementConfigs() const {
    return m_measurementConfigs;
  }
  QList<AcquisitionMode> acquisitionModes() const { return m_acquisitionModes; }

 private:
  QString m_name;
  QString m_description;
  Plugin m_plugin;
  QString m_interfaceCategory;
  QList<MeasurementConfig> m_measurementConfigs;
  QList<AcquisitionMode> m_acquisitionModes;
};

class OI_MAIN_EXPORT Tool {
 public:
  Tool() {}
  Tool(const QString& name, const QString& description, const Plugin& plugin)
      : m_name(name), m_description(description), m_plugin(plugin) {}

  QString name() const { return m_name; }
  QString description() const { return m_description; }
  Plugin plugin() const { return m_plugin; }
  QString qualifiedId() const { return m_plugin.filePath() + "#" + m_name; }

 private:
  QString m_name;
  QString m_description;
  Plugin m_plugin;
};

class OI_MAIN_EXPORT Exchange {
 public:
  Exchange() {}
  Exchange(const QString& name, const QString& description,
           const Plugin& plugin, const QString& interfaceCategory)
      : m_name(name),
        m_description(description),
        m_plugin(plugin),
        m_interfaceCategory(interfaceCategory) {}

  QString name() const { return m_name; }
  QString description() const { return m_description; }
  Plugin plugin() const { return m_plugin; }
  QString qualifiedId() const { return m_plugin.filePath() + "#" + m_name; }

  //! ExchangeSimpleAscii_iidd vs ExchangeDefinedFormat_iidd - NOT an identity
  //! (shared by every exchange of that category), kept only to distinguish
  //! the two exchange kinds the same way the old DB did
  QString interfaceCategory() const { return m_interfaceCategory; }

 private:
  QString m_name;
  QString m_description;
  Plugin m_plugin;
  QString m_interfaceCategory;
};

class OI_MAIN_EXPORT Simulation {
 public:
  Simulation() {}
  Simulation(const QString& name, const QString& description,
             const Plugin& plugin)
      : m_name(name), m_description(description), m_plugin(plugin) {}

  QString name() const { return m_name; }
  QString description() const { return m_description; }
  Plugin plugin() const { return m_plugin; }
  QString qualifiedId() const { return m_plugin.filePath() + "#" + m_name; }

 private:
  QString m_name;
  QString m_description;
  Plugin m_plugin;
};

class OI_MAIN_EXPORT Function {
 public:
  Function() {}
  Function(const QString& name, const QString& description,
           const Plugin& plugin, const QList<FeatureTypes>& applicableFor,
           const QList<ElementTypes>& neededElements,
           const QString& interfaceCategory)
      : m_name(name),
        m_description(description),
        m_plugin(plugin),
        m_applicableFor(applicableFor),
        m_neededElements(neededElements),
        m_interfaceCategory(interfaceCategory) {}

  QString name() const { return m_name; }
  QString description() const { return m_description; }
  Plugin plugin() const { return m_plugin; }
  QString qualifiedId() const { return m_plugin.filePath() + "#" + m_name; }
  QList<FeatureTypes> applicableFor() const { return m_applicableFor; }
  QList<ElementTypes> neededElements() const { return m_neededElements; }

  //! the function's Qt interface iid (ConstructFunction_iidd, FitFunction_iidd,
  //! ...) - NOT an identity (shared by every function of that category, see
  //! PluginRepository), kept only to classify "create" vs "change" functions
  //! the same way the old DB did
  QString interfaceCategory() const { return m_interfaceCategory; }

 private:
  QString m_name;
  QString m_description;
  Plugin m_plugin;
  QList<FeatureTypes> m_applicableFor;
  QList<ElementTypes> m_neededElements;
  QString m_interfaceCategory;
};

}  // namespace sys

#endif  // PLUGININFO_H

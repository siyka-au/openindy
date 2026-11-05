#ifndef SETTINGS_H
#define SETTINGS_H

#include <QMap>
#include <QString>

#if defined(OI_MAIN_LIB)
#define OI_MAIN_EXPORT Q_DECL_EXPORT
#else
#define OI_MAIN_EXPORT Q_DECL_IMPORT
#endif

/*!
 * \brief The Settings class
 * A VSCode-style layered key -> value store: Project overrides User
 * overrides System. "A default" isn't structurally special - it's just
 * the resolved value of a key like "defaultFunction.Plane" here, looked
 * up by whichever repository owns what that value refers to.
 *
 * System layer: config/defaults.json, shipped with the install, read-only
 * at runtime (this is what used to be relayed through SystemDbManager on
 * every startup - now read directly).
 * User layer: ~/.openindy/settings.json, read-write, persists across restarts.
 * Project layer: read-write, scoped to whichever project is open; in-memory
 * only for now (no project-file persistence hook exists yet for this).
 */
class OI_MAIN_EXPORT Settings {
 public:
  static void init();

  static QString get(const QString& key);

  static void setUserValue(const QString& key, const QString& value);
  static void setProjectValue(const QString& key, const QString& value);
  static void clearProjectValues();

  // loads the System layer from an explicit defaults.json path instead of
  // the production <appDir>/config/defaults.json - exposed for tests
  static void loadSystemLayerFromFile(const QString& path);

 private:
  static bool isInit;

  static QMap<QString, QString> systemLayer;
  static QMap<QString, QString> userLayer;
  static QMap<QString, QString> projectLayer;

  static void loadSystemLayer();
  static void loadUserLayer();
  static void saveUserLayer();

  static QString userSettingsFilePath();
};

#endif  // SETTINGS_H

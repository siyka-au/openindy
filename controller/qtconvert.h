#ifndef QTCONVERT_H
#define QTCONVERT_H

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QList>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVariantMap>
#include <QtCore>
#include <cstdint>
#include <filesystem>
#include <map>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "pluginmetadata.h"
#include "value.h"

// Conversions between the std containers of the core API and the Qt
// containers the Qt widgets, views and settings classes want.

template <typename T>
[[nodiscard]] QList<T> toQList(const std::vector<T>& v) {
  QList<T> out;
  out.reserve(static_cast<qsizetype>(v.size()));
  for (const T& e : v) {
    out.push_back(e);
  }
  return out;
}

template <typename T>
[[nodiscard]] std::vector<T> fromQList(const QList<T>& l) {
  return std::vector<T>(l.begin(), l.end());
}

// Text. The core holds UTF-8 std::string, Qt holds QString (UTF-16).
[[nodiscard]] inline QString toQString(std::string_view s) {
  return QString::fromUtf8(s.data(), static_cast<qsizetype>(s.size()));
}

[[nodiscard]] inline std::string toStd(const QString& s) {
  const QByteArray bytes = s.toUtf8();
  return std::string(bytes.constData(), static_cast<std::size_t>(bytes.size()));
}

[[nodiscard]] inline QStringList toQStringList(
    const std::vector<std::string>& v) {
  QStringList out;
  out.reserve(static_cast<qsizetype>(v.size()));
  for (const std::string& s : v) {
    out.push_back(toQString(s));
  }
  return out;
}

[[nodiscard]] inline std::vector<std::string> fromQStringList(
    const QStringList& l) {
  std::vector<std::string> out;
  out.reserve(static_cast<std::size_t>(l.size()));
  for (const QString& s : l) {
    out.push_back(toStd(s));
  }
  return out;
}

[[nodiscard]] inline QVariant toQVariant(const oi::Value& v) {
  if (v.isBool()) {
    return QVariant(v.toBool());
  }
  if (v.isNumber()) {
    return QVariant(v.toDouble());
  }
  if (v.isString()) {
    return QVariant(toQString(v.toString()));
  }
  return QVariant();
}

[[nodiscard]] inline oi::Value fromQVariant(const QVariant& v) {
  switch (v.typeId()) {
    case QMetaType::Bool:
      return oi::Value(v.toBool());
    case QMetaType::Int:
    case QMetaType::UInt:
    case QMetaType::LongLong:
    case QMetaType::ULongLong:
      return oi::Value(static_cast<std::int64_t>(v.toLongLong()));
    case QMetaType::Double:
    case QMetaType::Float:
      return oi::Value(v.toDouble());
    case QMetaType::QString:
      return oi::Value(toStd(v.toString()));
    default:
      return oi::Value();
  }
}

[[nodiscard]] inline QVariantMap toQVariantMap(const oi::ValueMap& m) {
  QVariantMap out;
  for (const auto& [key, value] : m) {
    out.insert(toQString(key), toQVariant(value));
  }
  return out;
}

[[nodiscard]] inline std::map<QString, QString> toQStringMap(
    const std::map<std::string, std::string>& m) {
  std::map<QString, QString> out;
  for (const auto& [key, value] : m) {
    out.insert_or_assign(toQString(key), toQString(value));
  }
  return out;
}

// Bytes (a serialized message), between std::string and QByteArray. Never
// assign one to the other directly: std::string = QByteArray compiles, and
// does something else.
[[nodiscard]] inline std::string toStdBytes(const QByteArray& bytes) {
  return std::string(bytes.constData(), static_cast<std::size_t>(bytes.size()));
}

[[nodiscard]] inline QByteArray toQByteArray(const std::string& bytes) {
  return QByteArray(bytes.data(), static_cast<qsizetype>(bytes.size()));
}

// The plugin's dependency list as read from its metadata (JSON array of
// objects with "name" and "path").
[[nodiscard]] inline std::vector<oi::PluginDependency> toDependencies(
    const QJsonArray& array) {
  std::vector<oi::PluginDependency> out;
  for (const QJsonValue& value : array) {
    const QJsonObject object = value.toObject();
    out.push_back({toStd(object.value("name").toString()),
                   toStd(object.value("path").toString())});
  }
  return out;
}

// The bundle template is JSON text in the core; the app works with the parsed
// object.
[[nodiscard]] inline QJsonObject toBundleTemplate(const std::string& json) {
  return QJsonDocument::fromJson(
             QByteArray(json.data(), static_cast<qsizetype>(json.size())))
      .object();
}

[[nodiscard]] inline std::string fromBundleTemplate(const QJsonObject& object) {
  if (object.isEmpty()) {
    return {};
  }
  const QByteArray bytes = QJsonDocument(object).toJson(QJsonDocument::Compact);
  return std::string(bytes.constData(), static_cast<std::size_t>(bytes.size()));
}

// std maps whose keys and values are text, between the core's std::string and
// the app's QString; plain values (numbers) pass through.
inline QString convertText(const std::string& s) { return toQString(s); }
inline std::string convertText(const QString& s) { return toStd(s); }
template <typename T>
  requires(!std::is_same_v<T, std::string> && !std::is_same_v<T, QString>)
T convertText(const T& v) {
  return v;
}

template <typename OutMap, typename InMap>
[[nodiscard]] OutMap convertMap(const InMap& in) {
  OutMap out;
  for (const auto& [key, value] : in) {
    out.emplace(convertText(key), convertText(value));
  }
  return out;
}

// File system paths, through UTF-16 so non-ASCII names survive on every
// platform.
[[nodiscard]] inline std::filesystem::path toPath(const QString& s) {
  return std::filesystem::path(s.toStdU16String());
}

[[nodiscard]] inline QString fromPath(const std::filesystem::path& p) {
  return QString::fromStdU16String(p.u16string());
}

#endif  // QTCONVERT_H

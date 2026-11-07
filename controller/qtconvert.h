#ifndef QTCONVERT_H
#define QTCONVERT_H

#include <QList>
#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <filesystem>
#include <map>
#include <vector>

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

[[nodiscard]] inline QStringList toQStringList(const std::vector<QString>& v) {
  return toQList(v);
}

[[nodiscard]] inline std::vector<QString> fromQStringList(
    const QStringList& l) {
  return fromQList(l);
}

[[nodiscard]] inline QVariantMap toQVariantMap(
    const std::map<QString, QVariant>& m) {
  QVariantMap out;
  for (const auto& [key, value] : m) {
    out.insert(key, value);
  }
  return out;
}

[[nodiscard]] inline std::map<QString, QVariant> fromQVariantMap(
    const QVariantMap& m) {
  std::map<QString, QVariant> out;
  for (auto it = m.cbegin(); it != m.cend(); ++it) {
    out.insert_or_assign(it.key(), it.value());
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

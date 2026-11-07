#ifndef READINGTABLECOLUMNCONFIG_H
#define READINGTABLECOLUMNCONFIG_H

#include <QList>
#include <vector>

#include "types.h"

using namespace oi;

/*!
 * \brief The ReadingTableColumnConfig class
 * Used to specify which attributes of a reading shall be displayed
 */
class ReadingTableColumnConfig {
 public:
  ReadingTableColumnConfig();

  [[nodiscard]] bool getColumnVisibility(
      const ReadingDisplayAttributes& column) const;
  void setColumnVisibility(const ReadingDisplayAttributes& column,
                           const bool& isVisible);

  [[nodiscard]] int getColumnPosition(
      const ReadingDisplayAttributes& column) const;
  void setColumnPosition(const ReadingDisplayAttributes& column,
                         const int& index);

 private:
  void init();

  std::vector<ReadingDisplayAttributes> displayColumns;
};

#endif  // READINGTABLECOLUMNCONFIG_H

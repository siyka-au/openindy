#ifndef OI_APP_CONTROLLER_CONFIG_READING_TABLE_COLUMN_CONFIG_H_
#define OI_APP_CONTROLLER_CONFIG_READING_TABLE_COLUMN_CONFIG_H_

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

#endif  // OI_APP_CONTROLLER_CONFIG_READING_TABLE_COLUMN_CONFIG_H_

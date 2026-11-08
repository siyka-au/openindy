#ifndef OI_APP_CONTROLLER_CONFIG_OBSERVATION_TABLE_COLUMN_CONFIG_H_
#define OI_APP_CONTROLLER_CONFIG_OBSERVATION_TABLE_COLUMN_CONFIG_H_

#include <QList>
#include <vector>

#include "types.h"

using namespace oi;

/*!
 * \brief The ObservationTableColumnConfig class
 * Used to specify which attributes of an observation shall be displayed
 */
class ObservationTableColumnConfig {
 public:
  ObservationTableColumnConfig();

  [[nodiscard]] bool getColumnVisibility(
      const ObservationDisplayAttributes& column) const;
  void setColumnVisibility(const ObservationDisplayAttributes& column,
                           const bool& isVisible);

  [[nodiscard]] int getColumnPosition(
      const ObservationDisplayAttributes& column) const;
  void setColumnPosition(const ObservationDisplayAttributes& column,
                         const int& index);

 private:
  void init();

  std::vector<ObservationDisplayAttributes> displayColumns;
};

#endif  // OI_APP_CONTROLLER_CONFIG_OBSERVATION_TABLE_COLUMN_CONFIG_H_

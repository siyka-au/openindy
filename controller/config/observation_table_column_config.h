#ifndef OBSERVATIONTABLECOLUMNCONFIG_H
#define OBSERVATIONTABLECOLUMNCONFIG_H

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

#endif  // OBSERVATIONTABLECOLUMNCONFIG_H

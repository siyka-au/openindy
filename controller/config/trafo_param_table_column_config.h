#ifndef OI_APP_CONTROLLER_CONFIG_TRAFO_PARAM_TABLE_COLUMN_CONFIG_H_
#define OI_APP_CONTROLLER_CONFIG_TRAFO_PARAM_TABLE_COLUMN_CONFIG_H_

#include <QList>
#include <vector>

#include "types.h"

using namespace oi;

/*!
 * \brief The TrafoParamTableColumnConfig class
 * Used to specify which attributes of a feature shall be displayed (e.g. trafo
 * param table view)
 */
class TrafoParamTableColumnConfig {
 public:
  explicit TrafoParamTableColumnConfig(bool initBundle = false);

  [[nodiscard]] bool getColumnVisibility(
      const TrafoParamDisplayAttributes& column) const;
  void setColumnVisibility(const TrafoParamDisplayAttributes& column,
                           const bool& isVisible);

  [[nodiscard]] int getColumnPosition(
      const TrafoParamDisplayAttributes& column) const;
  void setColumnPosition(const TrafoParamDisplayAttributes& column,
                         const int& index);

  [[nodiscard]] TrafoParamDisplayAttributes getDisplayAttributeAt(
      const int& column) const;

 private:
  void init();
  void initBundle();

  std::vector<TrafoParamDisplayAttributes> displayColumns;
};

#endif  // OI_APP_CONTROLLER_CONFIG_TRAFO_PARAM_TABLE_COLUMN_CONFIG_H_

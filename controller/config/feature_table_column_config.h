#ifndef FEATURETABLECOLUMNCONFIG_H
#define FEATURETABLECOLUMNCONFIG_H

#include <QList>
#include <vector>

#include "types.h"

using namespace oi;

/*!
 * \brief The FeatureTableColumnConfig class
 * Used to specify which attributes of a feature shall be displayed (e.g.
 * feature table view)
 */
class FeatureTableColumnConfig {
 public:
  FeatureTableColumnConfig();
  explicit FeatureTableColumnConfig(std::vector<FeatureDisplayAttributes>);

  [[nodiscard]] bool getColumnVisibility(
      const FeatureDisplayAttributes& column) const;
  void setColumnVisibility(const FeatureDisplayAttributes& column,
                           const bool& isVisible);

  [[nodiscard]] int getColumnPosition(
      const FeatureDisplayAttributes& column) const;
  void setColumnPosition(const FeatureDisplayAttributes& column,
                         const int& index);

  [[nodiscard]] FeatureDisplayAttributes getDisplayAttributeAt(
      const int& column) const;

 private:
  void init();

  std::vector<FeatureDisplayAttributes> displayColumns;
};

#endif  // FEATURETABLECOLUMNCONFIG_H

#ifndef FEATURESORTER_H
#define FEATURESORTER_H

#include <QPointer>

#include "feature.h"
#include "featuretablesortingconfig.h"
#include "featurewrapper.h"
#include "oijob.h"

#if defined(OI_MAIN_LIB)
#define OI_MAIN_EXPORT Q_DECL_EXPORT
#else
#define OI_MAIN_EXPORT Q_DECL_IMPORT
#endif

using namespace oi;

/*!
 * \brief The FeatureSorter class
 * Sorts features according to sorting settings
 */
class OI_MAIN_EXPORT FeatureSorter {
 public:
  FeatureSorter();

  // ##########
  // comparison
  // ##########

  bool operator()(const int& left, const int& right);
  bool operator()(const oi::Guarded<FeatureWrapper>& left,
                  const oi::Guarded<FeatureWrapper>& right);

  // #############
  // sorting modes
  // #############

  enum class SortingMode {
    eSortStandard,              // sort by: id, isNominal
    eSortBy_Name_ActNom,        // sort by: featureName, isNominal
    eSortBy_Group_Name_ActNom,  // sort by: groupName, featureName, isNominal
  };

  // #################################
  // get or set sorting mode an config
  // #################################

  [[nodiscard]] SortingMode getSortingMode() const;
  void setSortingMode(SortingMode mode);

  [[nodiscard]] const FeatureTableSortingConfig getSortingConfig() const;
  void setSortingConfig(const FeatureTableSortingConfig& config);

  // ###################################
  // get or set the current OpenIndy job
  // ###################################

  [[nodiscard]] const oi::Guarded<OiJob>& getCurrentJob() const;
  void setCurrentJob(const oi::Guarded<OiJob>& job);

  // ####################
  // comparison functions
  // ####################

  [[nodiscard]] bool lessThan(const int& left, const int& right) const;
  [[nodiscard]] bool lessThan(const oi::Guarded<FeatureWrapper>& left,
                              const oi::Guarded<FeatureWrapper>& right) const;

 private:
  // #################
  // helper attributes
  // #################

  oi::Guarded<OiJob> job;

  // sorting mode and config
  SortingMode mode{SortingMode::eSortStandard};
  FeatureTableSortingConfig config;

  // ##############
  // helper methods
  // ##############

  [[nodiscard]] bool sortStandard(oi::Guarded<FeatureWrapper> left,
                                  oi::Guarded<FeatureWrapper> right) const;
  [[nodiscard]] bool sortBy_Name_ActNom(
      oi::Guarded<FeatureWrapper> left,
      oi::Guarded<FeatureWrapper> right) const;
  [[nodiscard]] bool sortBy_Group_Name(oi::Guarded<FeatureWrapper> left,
                                       oi::Guarded<FeatureWrapper> right) const;
};

#endif  // FEATURESORTER_H

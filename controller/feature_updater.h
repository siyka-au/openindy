#ifndef OI_APP_CONTROLLER_FEATURE_UPDATER_H_
#define OI_APP_CONTROLLER_FEATURE_UPDATER_H_

#include <QObject>
#include <QPointer>
#include <vector>

#include "console.h"
#include "coordinate_system.h"
#include "feature.h"
#include "feature_attributes.h"
#include "feature_wrapper.h"
#include "function.h"
#include "oi_job.h"
#include "plugin_manager.h"
#include "sort_list_by_name.h"
#include "trafo_controller.h"
#include "trafo_param.h"

using namespace oi;

/*!
 * \brief The FeatureUpdater class
 * Handles all recalculation tasks of features of an OpenIndy job
 */
class FeatureUpdater : public QObject {
  Q_OBJECT

 public:
  explicit FeatureUpdater(QObject* parent = nullptr);

 signals:

  // #####################################################
  // inform when a recalculation process has been finished
  // #####################################################

  void featuresRecalculated();
  void featureRecalculated(const int& featureId);
  void trafoParamRecalculated(const int& featureId);

 public:
  // ###################################
  // get or set the current OpenIndy job
  // ###################################

  [[nodiscard]] const oi::Guarded<OiJob>& getCurrentJob() const;
  void setCurrentJob(const oi::Guarded<OiJob>& job);

 public slots:

  // #####################
  // recalculation methods
  // #####################

  // recalculate features
  void recalcFeatureSet();
  void recalcFeature(const oi::Guarded<Feature>& feature);
  void recalcTrafoParam(const oi::Guarded<TrafoParam>& trafoParam);
  bool recalcBundle(const oi::Guarded<CoordinateSystem>& bundleSystem);

  // change the active coordinate system (transforms all observations to the
  // given coordinate system, if possible)
  void switchCoordinateSystem();

 private:
  // ##############
  // helper methods
  // ##############

  void connectJob();
  void disconnectJob();

  // recursive recalculation
  void recursiveFeatureRecalculation(const oi::Guarded<Feature>& feature);

  // set up input elements for trafo param features
  void setUpTrafoParamActualActual(
      const oi::Guarded<TrafoParam>& trafoParam,
      const oi::Guarded<SystemTransformation>& systemTransformation);
  void setUpTrafoParamActualNominal(
      const oi::Guarded<TrafoParam>& trafoParam,
      const oi::Guarded<SystemTransformation>& systemTransformation);
  void setUpTrafoParamNominalNominal(
      const oi::Guarded<TrafoParam>& trafoParam,
      const oi::Guarded<SystemTransformation>& systemTransformation);
  void setUpTrafoParamBundleNominal(
      const oi::Guarded<TrafoParam>& trafoParam,
      const oi::Guarded<SystemTransformation>& systemTransformation);

  // change the active coordinate system (does not transform observations)
  void switchCoordinateSystemWithoutTransformation(
      const oi::Guarded<CoordinateSystem>& destinationSystem);
  void transformAllObsToDestSystem(
      const oi::Guarded<CoordinateSystem>& destinationSystem);
  void recalcFeatureSetWithoutTransformation();
  void recalcFeatureWithoutTransformation(const oi::Guarded<Feature>& feature);
  // change with transformation for bundle
  void switchCoordinateSystemWithTransformation(
      const oi::Guarded<CoordinateSystem>& destinationSystem);

  void transformObsAndNominals(
      const oi::Guarded<CoordinateSystem>& destinationSystem);

  // copy geometry
  void copyGeometry(InputElement& newElement,
                    const oi::Guarded<FeatureWrapper>& oldElement,
                    const ElementTypes& type);

  // clear or save bundle results
  void clearBundleResults(const oi::Guarded<CoordinateSystem>& bundleSystem);
  void saveBundleResults(const oi::Guarded<CoordinateSystem>& bundleSystem);
  // void createBundleGeometries(const std::vector<BundleGeometry> &geometries,
  // const oi::Guarded<CoordinateSystem> &bundleSystem);
  void createBundleTransformations(
      std::vector<BundleTransformation>& transformations,
      const oi::Guarded<CoordinateSystem>& bundleSystem);
  // recalc bundle- trafoParam dependencies
  void recalcAllBundleTrafoDependencies(
      const oi::Guarded<CoordinateSystem>& bundleSystem);

  // ##################
  // general attributes
  // ##################

  oi::Guarded<OiJob> currentJob;

  TrafoController
      trafoController;  // handles all transformations to transform Observations
                        // in the right coordinate system
};

#endif  // OI_APP_CONTROLLER_FEATURE_UPDATER_H_

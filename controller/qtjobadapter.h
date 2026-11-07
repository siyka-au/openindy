#ifndef QTJOBADAPTER_H
#define QTJOBADAPTER_H

#include <QObject>
#include <QPointer>

#include "jobobserver.h"
#include "measurementconfig.h"
#include "types.h"

namespace oi {
class Feature;
class OiJob;
}  // namespace oi

/*!
 * Turns the plain oi::JobObserver callbacks of an OiJob back into the Qt
 * signals the Qt application connects to. There is exactly one adapter per
 * job, created on first use by of() and destroyed together with the job.
 * The signals carry exactly the names and signatures the job used to declare
 * itself.
 */
class QtJobAdapter : public QObject, public oi::JobObserver {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(QtJobAdapter)

 public:
  // the adapter of `job` (created and registered on first use), or nullptr
  // for a null job. The adapter lives in the job's thread.
  static QtJobAdapter* of(oi::OiJob* job);
  static QtJobAdapter* of(const oi::Guarded<oi::OiJob>& job);

  // number of live adapters (for tests)
  static int registrySize();

  ~QtJobAdapter() override;

 signals:

  void activeFeatureChanged();
  void activeStationChanged();
  void activeCoordinateSystemChanged();
  void featureSetChanged();
  void coordSystemSetChanged();
  void stationSetChanged();
  void trafoParamSetChanged();
  void geometrySetChanged();
  void availableGroupsChanged();
  void activeGroupChanged();
  void featureAttributesChanged();
  void featureNameChanged(const int& featureId, const QString& oldName);
  void featureGroupChanged(const int& featureId, const QString& oldGroup);
  void featureCommentChanged(const int& featureId);
  void featureIsUpdatedChanged(const int& featureId);
  void featureIsSolvedChanged(const int& featureId);
  void featureFunctionsChanged(const int& featureId);
  void featureUsedForChanged(const int& featureId);
  void featurePreviouslyNeededChanged(const int& featureId);
  void geometryIsCommonChanged(const int& featureId);
  void geometryRealisesChanged(const int& featureId);
  void geometryObservationsChanged(const int& featureId);
  void groupMembersChanged(const int& featureId);
  void geometryNominalSystemChanged(const int& featureId);
  void geometryStatisticChanged(const int& featureId);
  void geometrySimulationDataChanged(const int& featureId);
  void geometryMeasurementConfigChanged(const int& featureId,
                                        const QString& oldMConfig,
                                        const oi::MeasurementConfigKey oldKey);
  void systemObservationsChanged(const int& featureId, const int& obsId);
  void systemTrafoParamsChanged(const int& featureId);
  void systemsNominalsChanged(const int& featureId);
  void stationSensorChanged(const int& featureId);
  void trafoParamParametersChanged(const int& featureId);
  void trafoParamSystemsChanged(const int& featureId);
  void trafoParamIsUsedChanged(const int& featureId);
  void trafoParamIsDatumChanged(const int& featureId);
  void sendMessage(const QString& msg, const oi::MessageTypes& msgType,
                   const oi::MessageDestinations& msgDest =
                       oi::MessageDestinations::eConsoleMessage) const;
  void recalcFeature(const oi::Guarded<oi::Feature>& feature);
  void recalcFeatureSet();
  void featuresRecalculated();
  void featureRecalculated(const int& featureId);
  void trafoParamRecalculated(const int& featureId);

 public:
  void onJobDestroyed(oi::OiJob* job) override;

  void onActiveFeatureChanged() override;
  void onActiveStationChanged() override;
  void onActiveCoordinateSystemChanged() override;
  void onFeatureSetChanged() override;
  void onCoordSystemSetChanged() override;
  void onStationSetChanged() override;
  void onTrafoParamSetChanged() override;
  void onGeometrySetChanged() override;
  void onAvailableGroupsChanged() override;
  void onActiveGroupChanged() override;
  void onFeatureAttributesChanged() override;
  void onFeatureNameChanged(const int& featureId,
                            const std::string& oldName) override;
  void onFeatureGroupChanged(const int& featureId,
                             const std::string& oldGroup) override;
  void onFeatureCommentChanged(const int& featureId) override;
  void onFeatureIsUpdatedChanged(const int& featureId) override;
  void onFeatureIsSolvedChanged(const int& featureId) override;
  void onFeatureFunctionsChanged(const int& featureId) override;
  void onFeatureUsedForChanged(const int& featureId) override;
  void onFeaturePreviouslyNeededChanged(const int& featureId) override;
  void onGeometryIsCommonChanged(const int& featureId) override;
  void onGeometryRealisesChanged(const int& featureId) override;
  void onGeometryObservationsChanged(const int& featureId) override;
  void onGroupMembersChanged(const int& featureId) override;
  void onGeometryNominalSystemChanged(const int& featureId) override;
  void onGeometryStatisticChanged(const int& featureId) override;
  void onGeometrySimulationDataChanged(const int& featureId) override;
  void onGeometryMeasurementConfigChanged(
      const int& featureId, const std::string& oldMConfig,
      const oi::MeasurementConfigKey oldKey) override;
  void onSystemObservationsChanged(const int& featureId,
                                   const int& obsId) override;
  void onSystemTrafoParamsChanged(const int& featureId) override;
  void onSystemsNominalsChanged(const int& featureId) override;
  void onStationSensorChanged(const int& featureId) override;
  void onTrafoParamParametersChanged(const int& featureId) override;
  void onTrafoParamSystemsChanged(const int& featureId) override;
  void onTrafoParamIsUsedChanged(const int& featureId) override;
  void onTrafoParamIsDatumChanged(const int& featureId) override;
  void onSendMessage(const std::string& msg, const oi::MessageTypes& msgType,
                     const oi::MessageDestinations& msgDest) override;
  void onRecalcFeatureRequested(
      const oi::Guarded<oi::Feature>& feature) override;
  void onRecalcFeatureSetRequested() override;
  void onFeaturesRecalculated() override;
  void onFeatureRecalculated(const int& featureId) override;
  void onTrafoParamRecalculated(const int& featureId) override;

 private:
  QtJobAdapter() = default;
};

#endif  // QTJOBADAPTER_H

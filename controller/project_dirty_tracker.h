#ifndef OI_APP_CONTROLLER_PROJECT_DIRTY_TRACKER_H_
#define OI_APP_CONTROLLER_PROJECT_DIRTY_TRACKER_H_

#include <atomic>

#include "job_observer.h"

namespace oi {
class OiJob;
}  // namespace oi

/*!
 * Answers "has the project changed since it was last saved or loaded?" without
 * looking at the project: it is a flag that every notification about a change
 * of saved content sets (the JobObserver events that report what the project
 * file stores, see proto/openindy/project.proto in the core), and that the
 * host clears after a successful save or load.
 *
 * Flag semantics, not content equality: a change that is undone afterwards
 * still leaves the project marked as changed.
 *
 * What does not set the flag: anything the project file does not store (the
 * working group, recalculation bookkeeping, messages) and everything derived
 * from other changes (isSolved, which is also delivered late). The sensors'
 * streams and connection state never reach a JobObserver at all.
 *
 * Attached to one job at a time; detaches itself from the job when the tracker
 * or the job goes first. Qt-free; events may arrive on any thread, the flag is
 * atomic.
 */
class ProjectDirtyTracker : public oi::JobObserver {
 public:
  ProjectDirtyTracker() = default;
  ~ProjectDirtyTracker() override;

  // Starts tracking `job` (and stops tracking the previous one). The project
  // starts out clean: attaching is what happens to a freshly loaded or created
  // job. A null job just detaches.
  void attach(oi::OiJob* job);
  void detach();

  [[nodiscard]] bool isDirty() const noexcept { return this->dirty.load(); }
  void markClean() noexcept { this->dirty.store(false); }
  void markDirty() noexcept { this->dirty.store(true); }

 private:
  void onJobDestroyed(oi::OiJob* job) override;

  // the project's structure and what is active
  void onActiveFeatureChanged() override { this->markDirty(); }
  void onActiveStationChanged() override { this->markDirty(); }
  void onActiveCoordinateSystemChanged() override { this->markDirty(); }
  void onFeatureSetChanged() override { this->markDirty(); }
  void onCoordSystemSetChanged() override { this->markDirty(); }
  void onStationSetChanged() override { this->markDirty(); }
  void onTrafoParamSetChanged() override { this->markDirty(); }
  void onGeometrySetChanged() override { this->markDirty(); }

  // features
  void onFeatureNameChanged(const int&, const std::string&) override {
    this->markDirty();
  }
  void onFeatureGroupChanged(const int&, const std::string&) override {
    this->markDirty();
  }
  void onFeatureCommentChanged(const int&) override { this->markDirty(); }
  void onFeatureFunctionsChanged(const int&) override { this->markDirty(); }
  void onFeatureUsedForChanged(const int&) override { this->markDirty(); }
  void onFeaturePreviouslyNeededChanged(const int&) override {
    this->markDirty();
  }

  // geometries and groups
  void onGeometryIsCommonChanged(const int&) override { this->markDirty(); }
  void onGeometryRealisesChanged(const int&) override { this->markDirty(); }
  void onGeometryObservationsChanged(const int&) override { this->markDirty(); }
  void onGeometryNominalSystemChanged(const int&) override {
    this->markDirty();
  }
  void onGeometryStatisticChanged(const int&) override { this->markDirty(); }
  void onGeometryParametersChanged(const int&) override { this->markDirty(); }
  void onGeometryOwnerFeatureChanged(const int&) override { this->markDirty(); }
  void onGeometryMeasurementConfigChanged(
      const int&, const std::string&, const oi::MeasurementConfigKey) override {
    this->markDirty();
  }
  void onGroupMembersChanged(const int&) override { this->markDirty(); }

  // coordinate systems, stations, transformations
  void onSystemObservationsChanged(const int&, const int&) override {
    this->markDirty();
  }
  void onSystemTrafoParamsChanged(const int&) override { this->markDirty(); }
  void onSystemsNominalsChanged(const int&) override { this->markDirty(); }
  void onSystemBundleChanged(const int&) override { this->markDirty(); }
  void onSystemFrameChanged(const int&) override { this->markDirty(); }
  void onStationSensorChanged(const int&) override { this->markDirty(); }
  void onTrafoParamParametersChanged(const int&) override { this->markDirty(); }
  void onTrafoParamSystemsChanged(const int&) override { this->markDirty(); }
  void onTrafoParamIsUsedChanged(const int&) override { this->markDirty(); }
  void onTrafoParamIsDatumChanged(const int&) override { this->markDirty(); }

  // functions
  void onFunctionContentChanged() override { this->markDirty(); }

  std::atomic<bool> dirty{false};
  oi::OiJob* job = nullptr;  // non-owning
};

#endif  // OI_APP_CONTROLLER_PROJECT_DIRTY_TRACKER_H_

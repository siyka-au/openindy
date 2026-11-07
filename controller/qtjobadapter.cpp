#include "qtjobadapter.h"

#include <QHash>
#include <QMutex>
#include <QMutexLocker>
#include <QThread>

#include "oijob.h"

using namespace oi;

namespace {
QMutex& registryMutex() {
  static QMutex mutex;
  return mutex;
}
QHash<const OiJob*, QtJobAdapter*>& registry() {
  static QHash<const OiJob*, QtJobAdapter*> r;
  return r;
}
}  // namespace

QtJobAdapter* QtJobAdapter::of(OiJob* job) {
  if (job == nullptr) {
    return nullptr;
  }
  QMutexLocker locker(&registryMutex());
  if (auto it = registry().constFind(job); it != registry().constEnd()) {
    return it.value();
  }
  auto* adapter = new QtJobAdapter;
  // no QObject parent: the job may be used from another thread than the one
  // asking for the adapter, so move it next to the job instead
  if (adapter->thread() != job->thread()) {
    adapter->moveToThread(job->thread());
  }
  registry().insert(job, adapter);
  job->addObserver(adapter);
  return adapter;
}

QtJobAdapter* QtJobAdapter::of(const QPointer<OiJob>& job) {
  return of(job.data());
}

int QtJobAdapter::registrySize() {
  QMutexLocker locker(&registryMutex());
  return static_cast<int>(registry().size());
}

QtJobAdapter::~QtJobAdapter() {
  QMutexLocker locker(&registryMutex());
  for (auto it = registry().begin(); it != registry().end();) {
    it = (it.value() == this) ? registry().erase(it) : std::next(it);
  }
}

// the job is going away: forget it and end the adapter's life with it
void QtJobAdapter::onJobDestroyed(OiJob* job) {
  {
    QMutexLocker locker(&registryMutex());
    registry().remove(job);
  }
  job->removeObserver(this);
  if (QThread::currentThread() == this->thread()) {
    delete this;
  } else {
    this->deleteLater();
  }
}

void QtJobAdapter::onActiveFeatureChanged() {
  emit this->activeFeatureChanged();
}

void QtJobAdapter::onActiveStationChanged() {
  emit this->activeStationChanged();
}

void QtJobAdapter::onActiveCoordinateSystemChanged() {
  emit this->activeCoordinateSystemChanged();
}

void QtJobAdapter::onFeatureSetChanged() { emit this->featureSetChanged(); }

void QtJobAdapter::onCoordSystemSetChanged() {
  emit this->coordSystemSetChanged();
}

void QtJobAdapter::onStationSetChanged() { emit this->stationSetChanged(); }

void QtJobAdapter::onTrafoParamSetChanged() {
  emit this->trafoParamSetChanged();
}

void QtJobAdapter::onGeometrySetChanged() { emit this->geometrySetChanged(); }

void QtJobAdapter::onAvailableGroupsChanged() {
  emit this->availableGroupsChanged();
}

void QtJobAdapter::onActiveGroupChanged() { emit this->activeGroupChanged(); }

void QtJobAdapter::onFeatureAttributesChanged() {
  emit this->featureAttributesChanged();
}

void QtJobAdapter::onFeatureNameChanged(const int& featureId,
                                        const QString& oldName) {
  emit this->featureNameChanged(featureId, oldName);
}

void QtJobAdapter::onFeatureGroupChanged(const int& featureId,
                                         const QString& oldGroup) {
  emit this->featureGroupChanged(featureId, oldGroup);
}

void QtJobAdapter::onFeatureCommentChanged(const int& featureId) {
  emit this->featureCommentChanged(featureId);
}

void QtJobAdapter::onFeatureIsUpdatedChanged(const int& featureId) {
  emit this->featureIsUpdatedChanged(featureId);
}

void QtJobAdapter::onFeatureIsSolvedChanged(const int& featureId) {
  emit this->featureIsSolvedChanged(featureId);
}

void QtJobAdapter::onFeatureFunctionsChanged(const int& featureId) {
  emit this->featureFunctionsChanged(featureId);
}

void QtJobAdapter::onFeatureUsedForChanged(const int& featureId) {
  emit this->featureUsedForChanged(featureId);
}

void QtJobAdapter::onFeaturePreviouslyNeededChanged(const int& featureId) {
  emit this->featurePreviouslyNeededChanged(featureId);
}

void QtJobAdapter::onGeometryIsCommonChanged(const int& featureId) {
  emit this->geometryIsCommonChanged(featureId);
}

void QtJobAdapter::onGeometryRealisesChanged(const int& featureId) {
  emit this->geometryRealisesChanged(featureId);
}

void QtJobAdapter::onGeometryObservationsChanged(const int& featureId) {
  emit this->geometryObservationsChanged(featureId);
}

void QtJobAdapter::onGroupMembersChanged(const int& featureId) {
  emit this->groupMembersChanged(featureId);
}

void QtJobAdapter::onGeometryNominalSystemChanged(const int& featureId) {
  emit this->geometryNominalSystemChanged(featureId);
}

void QtJobAdapter::onGeometryStatisticChanged(const int& featureId) {
  emit this->geometryStatisticChanged(featureId);
}

void QtJobAdapter::onGeometrySimulationDataChanged(const int& featureId) {
  emit this->geometrySimulationDataChanged(featureId);
}

void QtJobAdapter::onGeometryMeasurementConfigChanged(
    const int& featureId, const QString& oldMConfig,
    const MeasurementConfigKey oldKey) {
  emit this->geometryMeasurementConfigChanged(featureId, oldMConfig, oldKey);
}

void QtJobAdapter::onSystemObservationsChanged(const int& featureId,
                                               const int& obsId) {
  emit this->systemObservationsChanged(featureId, obsId);
}

void QtJobAdapter::onSystemTrafoParamsChanged(const int& featureId) {
  emit this->systemTrafoParamsChanged(featureId);
}

void QtJobAdapter::onSystemsNominalsChanged(const int& featureId) {
  emit this->systemsNominalsChanged(featureId);
}

void QtJobAdapter::onStationSensorChanged(const int& featureId) {
  emit this->stationSensorChanged(featureId);
}

void QtJobAdapter::onTrafoParamParametersChanged(const int& featureId) {
  emit this->trafoParamParametersChanged(featureId);
}

void QtJobAdapter::onTrafoParamSystemsChanged(const int& featureId) {
  emit this->trafoParamSystemsChanged(featureId);
}

void QtJobAdapter::onTrafoParamIsUsedChanged(const int& featureId) {
  emit this->trafoParamIsUsedChanged(featureId);
}

void QtJobAdapter::onTrafoParamIsDatumChanged(const int& featureId) {
  emit this->trafoParamIsDatumChanged(featureId);
}

void QtJobAdapter::onSendMessage(const QString& msg,
                                 const MessageTypes& msgType,
                                 const MessageDestinations& msgDest) {
  emit this->sendMessage(msg, msgType, msgDest);
}

void QtJobAdapter::onRecalcFeatureRequested(const QPointer<Feature>& feature) {
  emit this->recalcFeature(feature);
}

void QtJobAdapter::onRecalcFeatureSetRequested() {
  emit this->recalcFeatureSet();
}

void QtJobAdapter::onFeaturesRecalculated() {
  emit this->featuresRecalculated();
}

void QtJobAdapter::onFeatureRecalculated(const int& featureId) {
  emit this->featureRecalculated(featureId);
}

void QtJobAdapter::onTrafoParamRecalculated(const int& featureId) {
  emit this->trafoParamRecalculated(featureId);
}

#include "featuresorter.h"

FeatureSorter::FeatureSorter() {}

/*!
 * \brief FeatureSorter::operator ()
 * \param left
 * \param right
 * \return
 */
bool FeatureSorter::operator()(const int& left, const int& right) {
  return this->lessThan(left, right);
}

/*!
 * \brief FeatureSorter::operator ()
 * \param left
 * \param right
 * \return
 */
bool FeatureSorter::operator()(const QPointer<FeatureWrapper>& left,
                               const QPointer<FeatureWrapper>& right) {
  return this->lessThan(left, right);
}

/*!
 * \brief FeatureSorter::getSortingMode
 * \return
 */
FeatureSorter::SortingMode FeatureSorter::getSortingMode() const {
  return this->mode;
}

/*!
 * \brief FeatureSorter::setSortingMode
 * \param mode
 */
void FeatureSorter::setSortingMode(FeatureSorter::SortingMode mode) {
  this->mode = mode;
}

/*!
 * \brief FeatureSorter::getSortingConfig
 * \return
 */
const FeatureTableSortingConfig FeatureSorter::getSortingConfig() const {
  return this->config;
}

/*!
 * \brief FeatureSorter::setSortingConfig
 * \param config
 */
void FeatureSorter::setSortingConfig(const FeatureTableSortingConfig& config) {
  this->config = config;
}

/*!
 * \brief FeatureSorter::getCurrentJob
 * \return
 */
const QPointer<OiJob>& FeatureSorter::getCurrentJob() const {
  return this->job;
}

/*!
 * \brief FeatureSorter::setCurrentJob
 * \param job
 */
void FeatureSorter::setCurrentJob(const QPointer<OiJob>& job) {
  if (!job.isNull()) {
    this->job = job;
  }
}

/*!
 * \brief FeatureSorter::lessThan
 * \param left
 * \param right
 * \return
 */
bool FeatureSorter::lessThan(const int& left, const int& right) const {
  // check job
  if (this->job.isNull()) {
    return false;
  }

  // get features by id
  QPointer<FeatureWrapper> leftFeature = this->job->getFeatureById(left);
  QPointer<FeatureWrapper> rightFeature = this->job->getFeatureById(right);

  return this->lessThan(leftFeature, rightFeature);
}

/*!
 * \brief FeatureSorter::lessThan
 * \param left
 * \param right
 * \return
 */
bool FeatureSorter::lessThan(const QPointer<FeatureWrapper>& left,
                             const QPointer<FeatureWrapper>& right) const {
  // check features
  if (left.isNull() || left->getFeature().isNull() || right.isNull() ||
      right->getFeature().isNull()) {
    return false;
  }

  // call the sorting method specified by mode
  switch (this->mode) {
    case eSortStandard:
      return this->sortStandard(left, right);
    case eSortBy_Name_ActNom:
      return this->sortBy_Name_ActNom(left, right);
    case eSortBy_Group_Name_ActNom:
      return this->sortBy_Group_Name(left, right);
  }

  return false;
}

/*!
 * \brief FeatureSorter::isNominalLike
 * D17: nominal/actual is no longer a stored flag, so "isNominal" for
 * sorting purposes is read from ownership (D19) - a user-owned geometry
 * (no fit) sorts as nominal, a fit-owned one as actual.
 */
static bool isNominalLike(const QPointer<FeatureWrapper>& fw) {
  return !fw.isNull() && !fw->getGeometry().isNull() &&
         !fw->getGeometry()->getOwnerDescription().has_value();
}

/*!
 * \brief FeatureSorter::sortStandard
 * Sorts by id, i.e. job/insertion order. D17: a nominal and its
 * actual(s) are no longer name-matched pairs kept artificially adjacent
 * by id juggling when their names differ - each is an ordinary feature
 * added wherever it was created, same as any other feature.
 * \param left
 * \param right
 * \return
 */
bool FeatureSorter::sortStandard(QPointer<FeatureWrapper> left,
                                 QPointer<FeatureWrapper> right) const {
  return left->getFeature()->getId() < right->getFeature()->getId();
}

bool FeatureSorter::sortBy_Name_ActNom(QPointer<FeatureWrapper> left,
                                       QPointer<FeatureWrapper> right) const {
  if (left->getFeature()->getFeatureName().compare(
          right->getFeature()->getFeatureName(), Qt::CaseInsensitive) < 0) {
    return true;
  } else if (left->getFeature()->getFeatureName().compare(
                 right->getFeature()->getFeatureName(), Qt::CaseInsensitive) ==
             0) {
    return isNominalLike(left) < isNominalLike(right);
  }

  return false;
}

bool FeatureSorter::sortBy_Group_Name(QPointer<FeatureWrapper> left,
                                      QPointer<FeatureWrapper> right) const {
  if (left->getFeature()->getGroupName().compare(
          right->getFeature()->getGroupName(), Qt::CaseInsensitive) < 0) {
    return true;
  } else if (left->getFeature()->getGroupName().compare(
                 right->getFeature()->getGroupName(), Qt::CaseInsensitive) ==
             0) {
    if (left->getFeature()->getFeatureName().compare(
            right->getFeature()->getFeatureName(), Qt::CaseInsensitive) < 0) {
      return true;
    } else if (left->getFeature()->getFeatureName().compare(
                   right->getFeature()->getFeatureName(),
                   Qt::CaseInsensitive) == 0) {
      return isNominalLike(left) < isNominalLike(right);
    }
  }

  return false;
}

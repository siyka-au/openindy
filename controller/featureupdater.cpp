#include "featureupdater.h"

#include <map>
#include <vector>

#include "containers.h"
#include "jobconnect.h"
#include "qtconvert.h"
#include "qtjobadapter.h"

/*!
 * \brief FeatureUpdater::FeatureUpdater
 * \param parent
 */
FeatureUpdater::FeatureUpdater(QObject* parent) : QObject(parent) {}

/*!
 * \brief FeatureUpdater::getCurrentJob
 * \return
 */
const oi::Guarded<OiJob>& FeatureUpdater::getCurrentJob() const {
  return this->currentJob;
}

/*!
 * \brief FeatureUpdater::setCurrentJob
 * \param job
 */
void FeatureUpdater::setCurrentJob(const oi::Guarded<OiJob>& job) {
  if (!job.isNull()) {
    // disconnect current job
    if (!this->currentJob.isNull()) {
      this->disconnectJob();
    }

    this->currentJob = job;
    this->connectJob();
  }
}

/*!
 * \brief FeatureUpdater::recalcFeatureSet
 * Recalculate the hole set of features of the current OpenIndy job
 */
void FeatureUpdater::recalcFeatureSet() {
  // check job
  if (this->currentJob.isNull()) {
    return;
  }

  // check active coordinate system
  if (this->currentJob->getActiveCoordinateSystem().isNull()) {
    return;
  }

  // set all features to not have been updated - except function-owned
  // sub-geometry (Stage 7h, D19 applied to functions), which is left
  // however clean/dirty it already is. A freshly loaded owned accessor
  // was marked clean by ProjectExchanger (its persisted value is still
  // current); Geometry::recalc() is what actually dirties one again,
  // the moment its owner's own value changes. Resetting it here
  // unconditionally, like any ordinary feature, would re-execute its
  // construction chain on every recalc pass including the one that
  // follows every project load - exactly what domain-model.md S1/S6
  // says must not happen.
  foreach (const oi::Guarded<FeatureWrapper>& feature,
           this->currentJob->getFeaturesList()) {
    if (!feature.isNull() && !feature->getFeature().isNull()) {
      if (!feature->getGeometry().isNull() &&
          feature->getGeometry()->getOwnerFeature().has_value()) {
        continue;
      }
      feature->getFeature()->blockSignals(true);
      feature->getFeature()->setIsUpdated(false);
      feature->getFeature()->blockSignals(false);
    }
  }

  // recalc recursively (block signals during recalculation, one signal for all
  // features is emitted at the end)
  this->blockSignals(true);
  foreach (const oi::Guarded<FeatureWrapper>& feature,
           this->currentJob->getFeaturesList()) {
    // check feature
    if (feature.isNull() || feature->getFeature().isNull()) {
      continue;
    }

    // recalc the feature if it was not recalced yet and is no trafo param
    if (!feature->getFeature()->getIsUpdated() &&
        feature->getFeatureTypeEnum() != FeatureTypes::eTrafoParamFeature) {
      // this->recalcFeature(feature->getFeature());
      this->recursiveFeatureRecalculation(feature->getFeature());
    }
  }
  this->blockSignals(false);

  emit this->featuresRecalculated();
}

/*!
 * \brief FeatureUpdater::recalcFeature
 * Recalculates a single feature of an OpenIndy job and all its dependencies
 * \param feature
 */
void FeatureUpdater::recalcFeature(const oi::Guarded<Feature>& feature) {
  // check job
  if (this->currentJob.isNull()) {
    return;
  }

  // check active coordinate system
  if (this->currentJob->getActiveCoordinateSystem().isNull()) {
    return;
  }

  // check the feature
  if (feature.isNull() || feature->getFeatureWrapper().isNull() ||
      !feature->getFeatureWrapper()->getTrafoParam().isNull()) {
    return;
  }

  // transform all observations of the feature to the active coordinate system
  this->trafoController.transformObservations(
      feature, this->currentJob->getActiveCoordinateSystem());

  // set all features to not have been updated - except function-owned
  // sub-geometry; see recalcFeatureSet()'s comment
  foreach (const oi::Guarded<FeatureWrapper>& feature,
           this->currentJob->getFeaturesList()) {
    if (!feature.isNull() && !feature->getFeature().isNull()) {
      if (!feature->getGeometry().isNull() &&
          feature->getGeometry()->getOwnerFeature().has_value()) {
        continue;
      }
      feature->getFeature()->blockSignals(true);
      feature->getFeature()->setIsUpdated(false);
      feature->getFeature()->blockSignals(false);
    }
  }

  try {
    // recalculate feature
    this->recursiveFeatureRecalculation(feature);

    emit this->featureRecalculated(feature->getId());

  } catch (const exception& e) {
    emit this->currentJob->sendMessage(e.what(), MessageTypes::eErrorMessage);
  }
}

/*!
 * \brief FeatureUpdater::recalcTrafoParam
 * Recalculates a transformation parameter feature
 * \param trafoParam
 */
void FeatureUpdater::recalcTrafoParam(
    const oi::Guarded<TrafoParam>& trafoParam) {
  // check trafo param
  if (trafoParam.isNull() || trafoParam->getStartSystem().isNull() ||
      trafoParam->getDestinationSystem().isNull()) {
    return;
  }

  // get and check system transformation
  if (trafoParam->getFunctions().size() == 0 ||
      trafoParam->getFunctions().at(0).isNull()) {
    return;
  }
  oi::Guarded<SystemTransformation> systemTransformation =
      dynamic_cast<SystemTransformation*>(
          trafoParam->getFunctions().at(0).data());
  if (systemTransformation.isNull()) {
    return;
  }

  // set up input feature
  if (trafoParam->getStartSystem()->getIsStationSystem() &&
      trafoParam->getDestinationSystem()
          ->getIsStationSystem()) {  // actual - actual
    this->setUpTrafoParamActualActual(trafoParam, systemTransformation);
  } else if ((trafoParam->getStartSystem()->getIsStationSystem() &&
              !trafoParam->getDestinationSystem()->getIsStationSystem()) ||
             (!trafoParam->getStartSystem()->getIsStationSystem() &&
              trafoParam->getDestinationSystem()
                  ->getIsStationSystem())) {  // actual - nominal
    this->setUpTrafoParamActualNominal(trafoParam, systemTransformation);
  } else if ((trafoParam->getStartSystem()->getIsBundleSystem() &&
              (!trafoParam->getDestinationSystem()->getIsStationSystem() &&
               !trafoParam->getDestinationSystem()->getIsBundleSystem())) ||
             (trafoParam->getDestinationSystem()->getIsBundleSystem() &&
              (!trafoParam->getStartSystem()->getIsStationSystem() &&
               !trafoParam->getStartSystem()
                    ->getIsBundleSystem()))) {  // bundle - nominal
    this->setUpTrafoParamBundleNominal(trafoParam, systemTransformation);
  } else {  // nominal - nominal
    this->setUpTrafoParamNominalNominal(trafoParam, systemTransformation);
  }

  // recalculate trafo param
  trafoParam->recalc();
  trafoParam->setIsUpdated(true);

  // recalc dependent features
  foreach (const oi::Guarded<FeatureWrapper>& dependentFeature,
           trafoParam->getUsedFor()) {
    // check feature
    if (dependentFeature.isNull() || dependentFeature->getFeature().isNull() ||
        !dependentFeature->getTrafoParam().isNull()) {
      continue;
    }

    this->recalcFeature(dependentFeature->getFeature());
  }

  // recalculate the whole feature set (because transformation parameters have
  // changed)
  this->switchCoordinateSystem();
}

/*!
 * \brief FeatureUpdater::recalcBundle
 * Recalculates a bundle adjustment
 * \param bundleSystem
 * \return
 */
bool FeatureUpdater::recalcBundle(
    const oi::Guarded<CoordinateSystem>& bundleSystem) {
  // check job
  if (this->currentJob.isNull()) {
    return false;
  }

  // check bundle system
  if (bundleSystem.isNull() || bundleSystem->getBundlePlugin().isNull()) {
    return false;
  }
  oi::Guarded<BundleAdjustment> bundlePlugin = bundleSystem->getBundlePlugin();

  // clear old bundle results
  this->clearBundleResults(bundleSystem);

  // check bundle stations
  std::vector<BundleStation> stations = bundlePlugin->getInputStations();
  if (stations.size() < 2) {
    return false;
  }

  // set up input geometries
  std::vector<BundleStation> inputStations;
  foreach (BundleStation inputStation, stations) {
    // get corresponding station feature
    oi::Guarded<FeatureWrapper> station =
        this->currentJob->getFeatureById(inputStation.id);
    if (station.isNull() || station->getStation().isNull() ||
        station->getStation()->getCoordinateSystem().isNull()) {
      continue;
    }
    qDebug() << "FeatureUpdater::recalcBundle station:"
             << station->getStation()->getFeatureName();
    // switch to station system
    this->transformAllObsToDestSystem(
        station->getStation()->getCoordinateSystem());
    this->recalcFeatureSetWithoutTransformation();

    // add station geometries
    inputStation.geometries.clear();
    std::vector<oi::Guarded<Geometry> > geometries =
        station->getStation()->getTargetGeometries();
    foreach (const oi::Guarded<Geometry>& geom, geometries) {
      if (!geom.isNull() && geom->getIsCommon() && geom->getIsSolved()) {
        BundleGeometry inputGeometry;
        inputGeometry.id = geom->getId();
        inputGeometry.parameters[GeometryParameters::eUnknownX] =
            geom->getPosition().getVector().getAt(0);
        inputGeometry.parameters[GeometryParameters::eUnknownY] =
            geom->getPosition().getVector().getAt(1);
        inputGeometry.parameters[GeometryParameters::eUnknownZ] =
            geom->getPosition().getVector().getAt(2);
        inputStation.geometries.push_back(inputGeometry);
        qDebug() << "FeatureUpdater::recalcBundle inputGeometry:"
                 << geom->getId();
      }
    }
    inputStations.push_back(inputStation);
  }

  // check input stations
  if (inputStations.size() < 2) {
    return false;
  }
  bool hasCommonPoints = false;
  foreach (BundleStation bStation, inputStations) {
    if (bStation.geometries.size() > 2) {
      hasCommonPoints = true;
      break;
    }
  }

  if (!hasCommonPoints) {
    return false;
  }

  if (QLoggingCategory::defaultCategory()->isDebugEnabled()) {
    foreach (BundleStation bs, inputStations) {
      qDebug() << "BundleStation.id=" << bs.id;
      foreach (BundleGeometry bg, bs.geometries) {
        qDebug() << "\tgeometries.id=" << bg.id;
      }
    }
  }

  // set up base station and job
  bundlePlugin->setInputStations(inputStations);
  bundlePlugin->setBaseStation(inputStations.at(0));
  bundlePlugin->setCurrentJob(this->currentJob);

  // calculate bundle
  if (!bundlePlugin->runBundle()) {
    this->switchCoordinateSystem();
    return false;
  }

  // save bundle results
  this->saveBundleResults(bundleSystem);

  // recalculate all features
  this->switchCoordinateSystem();

  // recalc all transformations that include the bundle system
  // except own station-> bundle transformations
  this->recalcAllBundleTrafoDependencies(bundleSystem);
  this->recalcFeatureSet();

  return true;
}

/*!
 * \brief FeatureUpdater::switchCoordinateSystem
 * Does all recalculation steps necessary when the active coordinate system has
 * changed
 */
void FeatureUpdater::switchCoordinateSystem() {
  // check current job
  if (this->currentJob.isNull()) {
    return;
  }

  try {
    this->transformObsAndNominals(
        this->currentJob->getActiveCoordinateSystem());

    // ###################
    // recalc all features
    // ###################

    this->recalcFeatureSet();

  } catch (const exception& e) {
    emit this->currentJob->sendMessage(e.what(), MessageTypes::eErrorMessage);
  }
}

/*!
 * \brief FeatureUpdater::connectJob
 */
void FeatureUpdater::connectJob() {
  connectToJob(this, &FeatureUpdater::featureRecalculated, this->currentJob,
               &OiJob::featureRecalculated);
  connectToJob(this, &FeatureUpdater::featuresRecalculated, this->currentJob,
               &OiJob::featuresRecalculated);
  connectToJob(this, &FeatureUpdater::trafoParamRecalculated, this->currentJob,
               &OiJob::trafoParamRecalculated);

  QObject::connect(QtJobAdapter::of(this->currentJob.data()),
                   &QtJobAdapter::recalcFeature, this,
                   &FeatureUpdater::recalcFeature, Qt::AutoConnection);
  QObject::connect(QtJobAdapter::of(this->currentJob.data()),
                   &QtJobAdapter::activeCoordinateSystemChanged, this,
                   &FeatureUpdater::switchCoordinateSystem, Qt::AutoConnection);
  QObject::connect(QtJobAdapter::of(this->currentJob.data()),
                   &QtJobAdapter::trafoParamParametersChanged, this,
                   &FeatureUpdater::switchCoordinateSystem, Qt::AutoConnection);
  QObject::connect(QtJobAdapter::of(this->currentJob.data()),
                   &QtJobAdapter::trafoParamIsUsedChanged, this,
                   &FeatureUpdater::switchCoordinateSystem, Qt::AutoConnection);
  QObject::connect(QtJobAdapter::of(this->currentJob.data()),
                   &QtJobAdapter::recalcFeatureSet, this,
                   &FeatureUpdater::recalcFeatureSet, Qt::AutoConnection);
}

/*!
 * \brief FeatureUpdater::disconnectJob
 */
void FeatureUpdater::disconnectJob() {
  disconnectFromJob(this);
  disconnectFromJob(this);
  disconnectFromJob(this);

  QObject::disconnect(QtJobAdapter::of(this->currentJob.data()),
                      &QtJobAdapter::recalcFeature, this,
                      &FeatureUpdater::recalcFeature);
  QObject::disconnect(QtJobAdapter::of(this->currentJob.data()),
                      &QtJobAdapter::activeCoordinateSystemChanged, this,
                      &FeatureUpdater::switchCoordinateSystem);
}

/*!
 * \brief FeatureUpdater::recursiveFeatureRecalculation
 * Is called on a feature to recursively recalculate the feature and all its
 * dependencies
 * \param feature
 */
void FeatureUpdater::recursiveFeatureRecalculation(
    const oi::Guarded<Feature>& feature) {
  // check wether the feature has already been updated
  if (feature->getIsUpdated()) {
    return;
  }

  // recalculate all needed features
  foreach (oi::Guarded<FeatureWrapper> neededFeature,
           feature->getPreviouslyNeeded()) {
    // check needed feature
    if (neededFeature.isNull() || neededFeature->getFeature().isNull() ||
        !neededFeature->getTrafoParam().isNull()) {
      continue;
    }

    // recalculate needed feature
    this->recursiveFeatureRecalculation(neededFeature->getFeature());
  }

  // check again wether the feature has already been updated
  if (feature->getIsUpdated()) {
    return;
  }

  // recalculate feature and mark it as updated
  feature->recalc();
  feature->setIsUpdated(true);

  // recalculate dependent features
  foreach (oi::Guarded<FeatureWrapper> dependentFeature,
           feature->getUsedFor()) {
    // check needed feature
    if (dependentFeature.isNull() || dependentFeature->getFeature().isNull() ||
        !dependentFeature->getTrafoParam().isNull()) {
      continue;
    }

    // recalculate dependent feature
    this->recursiveFeatureRecalculation(dependentFeature->getFeature());
  }
}

/*!
 * \brief FeatureUpdater::setUpTrafoParamActualActual
 * \param trafoParam
 * \param systemTransformation
 */
void FeatureUpdater::setUpTrafoParamActualActual(
    const oi::Guarded<TrafoParam>& trafoParam,
    const oi::Guarded<SystemTransformation>& systemTransformation) {
  // delete old copy elements
  systemTransformation->inputPointsStartSystem.clear();
  systemTransformation->inputPointsDestinationSystem.clear();
  std::vector<int> startKeys =
      oi::keys(systemTransformation->inputElementsStartSystem);
  foreach (const int& key, startKeys) {
    std::vector<InputElement> startElements =
        oi::value(systemTransformation->inputElementsStartSystem, key);
    foreach (const InputElement& element, startElements) {
      if (!element.geometry.isNull()) {
        delete element.geometry.data();
      }
    }
  }
  systemTransformation->inputElementsStartSystem.clear();
  std::vector<int> destKeys =
      oi::keys(systemTransformation->inputElementsDestinationSystem);
  foreach (const int& key, destKeys) {
    std::vector<InputElement> destElements =
        oi::value(systemTransformation->inputElementsDestinationSystem, key);
    foreach (const InputElement& element, destElements) {
      if (!element.geometry.isNull()) {
        delete element.geometry.data();
      }
    }
  }
  systemTransformation->inputElementsDestinationSystem.clear();

  // get and check keys
  bool isAlignment = false;
  std::vector<int> keys = oi::keys(systemTransformation->getInputElements());
  if (keys.size() <= 0) {  // no input elements
    return;
  } else if (keys.size() == 1) {  // normal transformation
    isAlignment = false;
  } else {  // alignment
    isAlignment = true;
  }

  // sort helper object which compares and sorts the list of start and target
  // points
  SortListByName sorter;

  // ###################
  // set up start system
  // ###################

  // switch coordinate system to "from" system
  this->switchCoordinateSystemWithoutTransformation(
      trafoParam->getStartSystem());

  // set up input elements for alignment transformations
  if (isAlignment) {
    // run through all keys and add start system elements
    std::vector<int> keys = oi::keys(systemTransformation->getInputElements());
    foreach (const int& key, keys) {
      std::vector<InputElement> newElements;

      // get input elements at the position key
      std::vector<InputElement> inputElements =
          oi::value(systemTransformation->getInputElements(), key);
      foreach (const InputElement& element, inputElements) {
        // check geometry
        if (element.geometry.isNull() ||
            element.geometry->getFeatureWrapper().isNull()) {
          continue;
        }

        // check if the geometry is an actual and is solved
        if (!element.geometry->getOwnerDescription().has_value() ||
            !element.geometry->getIsSolved()) {
          continue;
        }

        InputElement copyElement(element.id);
        this->copyGeometry(copyElement, element.geometry->getFeatureWrapper(),
                           element.typeOfElement);
        newElements.push_back(copyElement);
      }

      if (newElements.size() > 0) {
        systemTransformation->inputElementsStartSystem.insert_or_assign(
            key, newElements);
      }
    }
  }

  // set up input elements for normal transformations
  if (!isAlignment) {
    // get input elements at the position 0
    std::vector<InputElement> inputElements =
        oi::value(systemTransformation->getInputElements(), 0);
    foreach (const InputElement& element, inputElements) {
      // add all points
      if (element.typeOfElement == ElementTypes::ePointElement &&
          !element.point.isNull()) {
        if (element.point->getOwnerDescription().has_value() &&
            element.point->getIsSolved()) {
          Point p;
          p.setFeatureName(element.point->getFeatureName());
          p.setPoint(element.point->getPosition());
          sorter.addLocPoint(p);
        }
      }
    }
  }

  // #########################
  // set up destination system
  // #########################

  // switch system to "to" system
  this->switchCoordinateSystemWithoutTransformation(
      trafoParam->getDestinationSystem());

  // set up input elements for alignment transformations
  if (isAlignment) {
    // run through all keys and add destination system elements
    std::vector<int> keys = oi::keys(systemTransformation->getInputElements());
    foreach (const int& key, keys) {
      std::vector<InputElement> newElements;

      // get input elements at the position key
      std::vector<InputElement> inputElements =
          oi::value(systemTransformation->getInputElements(), key);
      foreach (const InputElement& element, inputElements) {
        // check geometry
        if (element.geometry.isNull() ||
            element.geometry->getFeatureWrapper().isNull()) {
          continue;
        }

        // check if the geometry is an actual and is solved
        if (!element.geometry->getOwnerDescription().has_value() ||
            !element.geometry->getIsSolved()) {
          continue;
        }

        InputElement copyElement(element.id);
        this->copyGeometry(copyElement, element.geometry->getFeatureWrapper(),
                           element.typeOfElement);
        newElements.push_back(copyElement);
      }

      if (newElements.size() > 0) {
        systemTransformation->inputElementsDestinationSystem.insert_or_assign(
            key, newElements);
      }
    }
  }

  // set up input elements for normal transformations
  if (!isAlignment) {
    // get input elements at the position 0
    std::vector<InputElement> inputElements =
        oi::value(systemTransformation->getInputElements(), 0);
    foreach (const InputElement& element, inputElements) {
      // add all points
      if (element.typeOfElement == ElementTypes::ePointElement &&
          !element.point.isNull()) {
        if (element.point->getOwnerDescription().has_value() &&
            element.point->getIsSolved()) {
          Point p;
          p.setFeatureName(element.point->getFeatureName());
          p.setPoint(element.point->getPosition());
          sorter.addRefPoint(p);
          // sorter.addLocPoint(Point(*element.point.data()));
        }
      }
    }
  }

  // #########################
  // reswitch to active system
  // #########################

  // add sorted lists to the function
  systemTransformation->inputPointsStartSystem =
      fromQList(sorter.getLocPoints());
  systemTransformation->inputPointsDestinationSystem =
      fromQList(sorter.getRefPoints());

  // switch back to the active system
  this->switchCoordinateSystem();
}

/*!
 * \brief FeatureUpdater::setUpTrafoParamActualNominal
 * \param trafoParam
 * \param systemTransformation
 */
void FeatureUpdater::setUpTrafoParamActualNominal(
    const oi::Guarded<TrafoParam>& trafoParam,
    const oi::Guarded<SystemTransformation>& systemTransformation) {
  // delete old copy elements
  systemTransformation->inputPointsStartSystem.clear();
  systemTransformation->inputPointsDestinationSystem.clear();
  std::vector<int> startKeys =
      oi::keys(systemTransformation->inputElementsStartSystem);
  foreach (const int& key, startKeys) {
    std::vector<InputElement> startElements =
        oi::value(systemTransformation->inputElementsStartSystem, key);
    foreach (const InputElement& element, startElements) {
      if (!element.geometry.isNull()) {
        delete element.geometry.data();
      }
    }
  }
  systemTransformation->inputElementsStartSystem.clear();
  std::vector<int> destKeys =
      oi::keys(systemTransformation->inputElementsDestinationSystem);
  foreach (const int& key, destKeys) {
    std::vector<InputElement> destElements =
        oi::value(systemTransformation->inputElementsDestinationSystem, key);
    foreach (const InputElement& element, destElements) {
      if (!element.geometry.isNull()) {
        delete element.geometry.data();
      }
    }
  }
  systemTransformation->inputElementsDestinationSystem.clear();

  // get and check keys
  bool isAlignment = false;
  std::vector<int> keys = oi::keys(systemTransformation->getInputElements());
  if (keys.size() <= 0) {  // no input elements
    return;
  } else if ((keys.size() == 1) ||
             (keys.size() == 2)) {  // normal transformation
    isAlignment = false;
  } else {  // alignment
    isAlignment = true;
  }

  // sort helper object which compares and sorts the list of start and target
  // points
  SortListByName sorter;

  // ###################
  // set up start system
  // ###################

  // switch to "from" system
  this->switchCoordinateSystemWithoutTransformation(
      trafoParam->getStartSystem());

  // set up input elements for alignment transformations
  if (isAlignment) {
    // run through all keys and add start system elements
    std::vector<int> keys = oi::keys(systemTransformation->getInputElements());
    foreach (const int& key, keys) {
      std::vector<InputElement> newElements;

      // get input elements at the position key
      std::vector<InputElement> inputElements =
          oi::value(systemTransformation->getInputElements(), key);
      foreach (const InputElement& element, inputElements) {
        // check geometry
        if (element.geometry.isNull() ||
            element.geometry->getFeatureWrapper().isNull()) {
          continue;
        }

        // check if the geometry is an actual and is solved
        if (!element.geometry->getOwnerDescription().has_value() ||
            !element.geometry->getIsSolved()) {
          continue;
        }

        InputElement copyElement(element.id);
        this->copyGeometry(copyElement, element.geometry->getFeatureWrapper(),
                           element.typeOfElement);
        newElements.push_back(copyElement);
      }

      if (newElements.size() > 0) {
        systemTransformation->inputElementsStartSystem.insert_or_assign(
            key, newElements);
      }
    }
  }

  // set up input elements for normal transformations
  if (!isAlignment) {
    // get input elements at the position 0
    std::vector<InputElement> inputElements =
        oi::value(systemTransformation->getInputElements(), 0);
    foreach (const InputElement& element, inputElements) {
      // add all points
      if (element.typeOfElement == ElementTypes::ePointElement &&
          !element.point.isNull()) {
        if ((!trafoParam->getStartSystem()->getIsStationSystem() &&
             !element.point->getOwnerDescription().has_value()) ||
            element.point->getIsSolved()) {
          Point p;
          p.setFeatureName(element.point->getFeatureName());
          p.setPoint(element.point->getPosition());
          sorter.addLocPoint(p);
        }
      }
    }
    // bestFit with level
    if (keys.size() == 2) {
      // get input elements at the position key
      std::vector<InputElement> newElements;

      // get input elements at the position key
      std::vector<InputElement> inputElements =
          oi::value(systemTransformation->getInputElements(), 1);
      foreach (const InputElement& element, inputElements) {
        // check geometry
        if (element.geometry.isNull() ||
            element.geometry->getFeatureWrapper().isNull()) {
          continue;
        }

        // check if the geometry is an actual and is solved
        if (!element.geometry->getOwnerDescription().has_value() ||
            !element.geometry->getIsSolved()) {
          continue;
        }
        Plane plane = *element.geometry->getFeatureWrapper()->getPlane();
        InputElement copyElement(element.id);
        this->copyGeometry(copyElement, element.geometry->getFeatureWrapper(),
                           element.typeOfElement);
        newElements.push_back(copyElement);
      }
      if (newElements.size() > 0) {
        systemTransformation->inputElementsStartSystem.insert_or_assign(
            1, newElements);
      }
    }
  }

  // #########################
  // set up destination system
  // #########################

  // switch to "to" system
  this->switchCoordinateSystemWithoutTransformation(
      trafoParam->getDestinationSystem());

  // set up input elements for alignment transformations
  if (isAlignment) {
    // run through all keys and add destination system elements
    std::vector<int> keys = oi::keys(systemTransformation->getInputElements());
    foreach (const int& key, keys) {
      std::vector<InputElement> newElements;

      // get input elements at the position key
      std::vector<InputElement> inputElements =
          oi::value(systemTransformation->getInputElements(), key);
      foreach (const InputElement& element, inputElements) {
        // check geometry
        if (element.geometry.isNull() ||
            element.geometry->getFeatureWrapper().isNull()) {
          continue;
        }

        // check if the geometry is an actual and is solved
        if (!element.geometry->getOwnerDescription().has_value() ||
            !element.geometry->getIsSolved()) {
          continue;
        }

        InputElement copyElement(element.id);
        this->copyGeometry(copyElement, element.geometry->getFeatureWrapper(),
                           element.typeOfElement);
        newElements.push_back(copyElement);
      }

      if (newElements.size() > 0) {
        systemTransformation->inputElementsDestinationSystem.insert_or_assign(
            key, newElements);
      }
    }
  }

  // set up input elements for normal transformations
  if (!isAlignment) {
    // get input elements at the position 0
    std::vector<InputElement> inputElements =
        oi::value(systemTransformation->getInputElements(), 0);
    foreach (const InputElement& element, inputElements) {
      // add all points
      if (element.typeOfElement == ElementTypes::ePointElement &&
          !element.point.isNull()) {
        if ((!trafoParam->getDestinationSystem()->getIsStationSystem() &&
             !element.point->getOwnerDescription().has_value()) ||
            element.point->getIsSolved()) {
          Point p;
          p.setFeatureName(element.point->getFeatureName());
          p.setPoint(element.point->getPosition());
          sorter.addRefPoint(p);
        }
      }
    }
    // bestFit with level
    if (keys.size() == 2) {
      // get input elements at the position key
      std::vector<InputElement> newElements;

      // get input elements at the position key
      std::vector<InputElement> inputElements =
          oi::value(systemTransformation->getInputElements(), 1);
      foreach (const InputElement& element, inputElements) {
        // check geometry
        if (element.geometry.isNull() ||
            element.geometry->getFeatureWrapper().isNull()) {
          continue;
        }

        // check if the geometry is an actual and is solved
        if (!element.geometry->getOwnerDescription().has_value() ||
            !element.geometry->getIsSolved()) {
          continue;
        }

        InputElement copyElement(element.id);
        this->copyGeometry(copyElement, element.geometry->getFeatureWrapper(),
                           element.typeOfElement);
        newElements.push_back(copyElement);
      }
      if (newElements.size() > 0) {
        systemTransformation->inputElementsDestinationSystem.insert_or_assign(
            1, newElements);
      }
    }
  }

  // #########################
  // reswitch to active system
  // #########################

  // add sorted lists to the function
  systemTransformation->inputPointsStartSystem =
      fromQList(sorter.getLocPoints());
  systemTransformation->inputPointsDestinationSystem =
      fromQList(sorter.getRefPoints());

  // switch back to active system
  this->switchCoordinateSystem();
}

/*!
 * \brief FeatureUpdater::setUpTrafoParamNominalNominal
 * \param trafoParam
 * \param systemTransformation
 */
void FeatureUpdater::setUpTrafoParamNominalNominal(
    const oi::Guarded<TrafoParam>& trafoParam,
    const oi::Guarded<SystemTransformation>& systemTransformation) {
  // delete old copy elements
  systemTransformation->inputPointsStartSystem.clear();
  systemTransformation->inputPointsDestinationSystem.clear();
  std::vector<int> startKeys =
      oi::keys(systemTransformation->inputElementsStartSystem);
  foreach (const int& key, startKeys) {
    std::vector<InputElement> startElements =
        oi::value(systemTransformation->inputElementsStartSystem, key);
    foreach (const InputElement& element, startElements) {
      if (!element.geometry.isNull()) {
        delete element.geometry.data();
      }
    }
  }
  systemTransformation->inputElementsStartSystem.clear();
  std::vector<int> destKeys =
      oi::keys(systemTransformation->inputElementsDestinationSystem);
  foreach (const int& key, destKeys) {
    std::vector<InputElement> destElements =
        oi::value(systemTransformation->inputElementsDestinationSystem, key);
    foreach (const InputElement& element, destElements) {
      if (!element.geometry.isNull()) {
        delete element.geometry.data();
      }
    }
  }
  systemTransformation->inputElementsDestinationSystem.clear();

  // TODO bool oder aehnliches verwenden und nicht keys

  // get and check keys
  bool isAlignment = false;
  std::vector<int> keys = oi::keys(systemTransformation->getInputElements());
  if (keys.size() <= 0) {  // no input elements
    return;
  } else if (keys.size() == 1) {  // normal transformation
    isAlignment = false;
  } else {  // alignment
    isAlignment = true;
  }

  // sort helper object which compares and sorts the list of start and target
  // points
  SortListByName sorter;

  // ###################
  // set up start system
  // ###################

  // set up input elements for normal transformations
  if (!isAlignment) {
    // get input elements at the position 0
    std::vector<InputElement> inputElements =
        oi::value(systemTransformation->getInputElements(), 0);
    foreach (const InputElement& element, inputElements) {
      // add all points
      if (element.typeOfElement == ElementTypes::ePointElement &&
          !element.point.isNull()) {
        if (!element.point->getOwnerDescription().has_value() &&
            element.point->getNominalSystem() == trafoParam->getStartSystem()) {
          Point p;
          p.setFeatureName(element.point->getFeatureName());
          p.setPoint(element.point->getPosition());
          sorter.addLocPoint(p);
        }
      }
    }
  }

  // #########################
  // set up destination system
  // #########################

  // if coord sys needs to be switched to "to" system
  this->switchCoordinateSystemWithoutTransformation(
      trafoParam->getDestinationSystem());

  // set up input elements for normal transformations
  if (!isAlignment) {
    // get input elements at the position 0
    std::vector<InputElement> inputElements =
        oi::value(systemTransformation->getInputElements(), 0);
    foreach (const InputElement& element, inputElements) {
      // add all points
      if (element.typeOfElement == ElementTypes::ePointElement &&
          !element.point.isNull()) {
        if (!element.point->getOwnerDescription().has_value() &&
            element.point->getNominalSystem() ==
                trafoParam->getDestinationSystem()) {
          Point p;
          p.setFeatureName(element.point->getFeatureName());
          p.setPoint(element.point->getPosition());
          sorter.addRefPoint(p);
        }
      }
    }
  }

  // add sorted lists to the function
  systemTransformation->inputPointsStartSystem =
      fromQList(sorter.getLocPoints());
  systemTransformation->inputPointsDestinationSystem =
      fromQList(sorter.getRefPoints());
}

/*!
 * \brief FeatureUpdater::setUpTrafoParamBundleNominal
 * \param trafoParam
 * \param systemTransformation
 */
void FeatureUpdater::setUpTrafoParamBundleNominal(
    const oi::Guarded<TrafoParam>& trafoParam,
    const oi::Guarded<SystemTransformation>& systemTransformation) {
  // delete old copy elements
  systemTransformation->inputPointsStartSystem.clear();
  systemTransformation->inputPointsDestinationSystem.clear();

  std::vector<int> startKeys =
      oi::keys(systemTransformation->inputElementsStartSystem);
  foreach (const int& key, startKeys) {
    std::vector<InputElement> startElements =
        oi::value(systemTransformation->inputElementsStartSystem, key);
    foreach (const InputElement& element, startElements) {
      if (!element.geometry.isNull()) {
        delete element.geometry.data();
      }
    }
  }
  systemTransformation->inputElementsStartSystem.clear();

  std::vector<int> destKeys =
      oi::keys(systemTransformation->inputElementsDestinationSystem);
  foreach (const int& key, destKeys) {
    std::vector<InputElement> destElements =
        oi::value(systemTransformation->inputElementsDestinationSystem, key);
    foreach (const InputElement& element, destElements) {
      if (!element.geometry.isNull()) {
        delete element.geometry.data();
      }
    }
  }
  systemTransformation->inputElementsDestinationSystem.clear();

  // check if this trafo already exists and temporary de activate the
  // transformation
  std::vector<oi::Guarded<TrafoParam> > existingTrafoParams =
      this->currentJob->getTransformationParametersList();

  std::vector<oi::Guarded<TrafoParam> > tempUnsolvedTRafoParam;

  // check if trafo param has the same configuration
  foreach (oi::Guarded<TrafoParam> existingTP, existingTrafoParams) {
    if ((existingTP->getStartSystem() == trafoParam->getStartSystem() &&
         existingTP->getDestinationSystem() ==
             trafoParam->getDestinationSystem()) ||
        (existingTP->getStartSystem() == trafoParam->getDestinationSystem() &&
         existingTP->getDestinationSystem() == trafoParam->getStartSystem())) {
      // if it is currently used, set it temporarly to unused
      if (existingTP->getIsUsed()) {
        existingTP->setIsUsed(false);
        tempUnsolvedTRafoParam.push_back(existingTP);
      }
    }
  }

  // get and check keys
  bool isAlignment = false;
  std::vector<int> keys = oi::keys(systemTransformation->getInputElements());
  if (keys.size() <= 0) {  // no input elements
    return;
  } else if (keys.size() == 1 || keys.size() == 2) {  // normal transformation
    isAlignment = false;
  } else {  // alignment
    isAlignment = true;
  }

  // sort helper object which compares and sorts the list of start and target
  // points
  SortListByName sorter;

  // ###################
  // set up start system
  // ###################

  // switch to "from" system
  this->switchCoordinateSystemWithTransformation(trafoParam->getStartSystem());

  // set up input elements for alignment transformations
  if (isAlignment) {
    // run through all keys and add start system elements
    std::vector<int> keys = oi::keys(systemTransformation->getInputElements());
    foreach (const int& key, keys) {
      std::vector<InputElement> newElements;

      // get input elements at the position key
      std::vector<InputElement> inputElements =
          oi::value(systemTransformation->getInputElements(), key);
      foreach (const InputElement& element, inputElements) {
        // check geometry
        if (element.geometry.isNull() ||
            element.geometry->getFeatureWrapper().isNull()) {
          continue;
        }

        // check if the geometry is an actual and is solved
        if (!element.geometry->getOwnerDescription().has_value() ||
            !element.geometry->getIsSolved()) {
          continue;
        }

        InputElement copyElement(element.id);
        this->copyGeometry(copyElement, element.geometry->getFeatureWrapper(),
                           element.typeOfElement);
        newElements.push_back(copyElement);
      }

      if (newElements.size() > 0) {
        systemTransformation->inputElementsStartSystem.insert_or_assign(
            key, newElements);
      }
    }
  }

  // set up input elements for normal transformation
  if (!isAlignment) {
    // get input elements at the position 0
    std::vector<InputElement> inputElements =
        oi::value(systemTransformation->getInputElements(), 0);
    foreach (const InputElement& element, inputElements) {
      // add all points
      if (element.typeOfElement == ElementTypes::ePointElement &&
          !element.point.isNull()) {
        if (!trafoParam->getStartSystem()->getIsStationSystem() &&
            element.point->getIsSolved()) {
          Point p;
          p.setFeatureName(element.point->getFeatureName());
          p.setPoint(element.point->getPosition());
          sorter.addLocPoint(p);
        }
      }
    }
    // bestFit with level
    if (keys.size() == 2) {
      // get input elements at the position key
      std::vector<InputElement> newElements;

      // get input elements at the position key
      std::vector<InputElement> inputElements =
          oi::value(systemTransformation->getInputElements(), 1);
      foreach (const InputElement& element, inputElements) {
        // check geometry
        if (element.geometry.isNull() ||
            element.geometry->getFeatureWrapper().isNull()) {
          continue;
        }

        // check if the geometry is an actual and is solved
        if (!element.geometry->getOwnerDescription().has_value() ||
            !element.geometry->getIsSolved()) {
          continue;
        }
        Plane plane = *element.geometry->getFeatureWrapper()->getPlane();
        InputElement copyElement(element.id);
        this->copyGeometry(copyElement, element.geometry->getFeatureWrapper(),
                           element.typeOfElement);
        newElements.push_back(copyElement);
      }
      if (newElements.size() > 0) {
        systemTransformation->inputElementsStartSystem.insert_or_assign(
            1, newElements);
      }
    }
  }

  // #########################
  // set up destination system
  // #########################

  this->switchCoordinateSystemWithTransformation(
      trafoParam->getDestinationSystem());

  // set up input elements for alignment transformation
  if (isAlignment) {
    // run through all keys and add destination system elements
    std::vector<int> keys = oi::keys(systemTransformation->getInputElements());
    foreach (const int& key, keys) {
      std::vector<InputElement> newElements;

      // get input elements at the position key
      std::vector<InputElement> inputElements =
          oi::value(systemTransformation->getInputElements(), key);
      foreach (const InputElement& element, inputElements) {
        // check geometry
        if (element.geometry.isNull() ||
            element.geometry->getFeatureWrapper().isNull()) {
          continue;
        }

        // check if the geometry is an actual and is solved
        if (!element.geometry->getOwnerDescription().has_value() ||
            !element.geometry->getIsSolved()) {
          continue;
        }

        InputElement copyElement(element.id);
        this->copyGeometry(copyElement, element.geometry->getFeatureWrapper(),
                           element.typeOfElement);
        newElements.push_back(copyElement);
      }

      if (newElements.size() > 0) {
        systemTransformation->inputElementsDestinationSystem.insert_or_assign(
            key, newElements);
      }
    }
  }

  // set up input elements for normal transformation

  if (!isAlignment) {
    // get input elements at the position 0
    std::vector<InputElement> inputElements =
        oi::value(systemTransformation->getInputElements(), 0);
    foreach (const InputElement& element, inputElements) {
      // add all points
      if (element.typeOfElement == ElementTypes::ePointElement &&
          !element.point.isNull()) {
        if (!trafoParam->getDestinationSystem()->getIsStationSystem() &&
            element.point->getIsSolved()) {
          Point p;
          p.setFeatureName(element.point->getFeatureName());
          p.setPoint(element.point->getPosition());
          sorter.addRefPoint(p);
        }
      }
    }
    if (keys.size() == 2) {
      // get input elements at the position key
      std::vector<InputElement> newElements;

      // get input elements at the position key
      std::vector<InputElement> inputElements =
          oi::value(systemTransformation->getInputElements(), 1);
      foreach (const InputElement& element, inputElements) {
        // check geometry
        if (element.geometry.isNull() ||
            element.geometry->getFeatureWrapper().isNull()) {
          continue;
        }

        // check if the geometry is an actual and is solved
        if (!element.geometry->getOwnerDescription().has_value() ||
            !element.geometry->getIsSolved()) {
          continue;
        }

        InputElement copyElement(element.id);
        this->copyGeometry(copyElement, element.geometry->getFeatureWrapper(),
                           element.typeOfElement);
        newElements.push_back(copyElement);
      }

      if (newElements.size() > 0) {
        systemTransformation->inputElementsDestinationSystem.insert_or_assign(
            1, newElements);
      }
    }
  }

  // #########################
  // reswitch to active system
  // #########################

  // add sorted lists to the function
  systemTransformation->inputPointsStartSystem =
      fromQList(sorter.getLocPoints());
  systemTransformation->inputPointsDestinationSystem =
      fromQList(sorter.getRefPoints());

  // re activate all existing transformations
  foreach (oi::Guarded<TrafoParam> tp, tempUnsolvedTRafoParam) {
    tp->setIsUsed(true);
  }

  // switch back to active system
  this->switchCoordinateSystem();
}

void FeatureUpdater::transformAllObsToDestSystem(
    const oi::Guarded<CoordinateSystem>& destinationSystem) {
  // check current job
  if (this->currentJob.isNull()) {
    return;
  }

  // check destination system
  if (destinationSystem.isNull()) {
    return;
  }

  // run through all station systems
  foreach (const oi::Guarded<Station>& station,
           this->currentJob->getStationsList()) {
    // check station system
    if (station.isNull() || station->getCoordinateSystem().isNull()) {
      continue;
    }

    const bool isSolved = (station->getCoordinateSystem() == destinationSystem);
    // run through all observations of the station system
    foreach (const oi::Guarded<Observation>& obs,
             station->getCoordinateSystem()->getObservations()) {
      // set observation to solved only if it has been measured in the active
      // coordinate system
      if (!obs.isNull()) {
        obs->setXYZ(obs->getOriginalXYZ());
        obs->setIJK(obs->getOriginalIJK());
        // uncertainty was already computed correctly (Reading propagates
        // polar sigma into cartesian sigma, setObservation() copies it into
        // originalSigmaXyz) but nothing ever carried it across into the
        // working sigmaXyz fit functions actually read - setSigmaXyz() had
        // no callers anywhere in the codebase before this
        obs->setSigmaXyz(obs->getOriginalSigmaXyz());
        obs->setSigmaIjk(obs->getOriginalSigmaIjk());
        obs->setIsSolved(isSolved);
      }
    }
  }
}

/*!
 * \brief FeatureUpdater::switchCoordinateSystemWithoutTransformation
 * \param destinationSystem
 */
void FeatureUpdater::switchCoordinateSystemWithoutTransformation(
    const oi::Guarded<CoordinateSystem>& destinationSystem) {
  // check current job
  if (this->currentJob.isNull()) {
    return;
  }

  // check destination system
  if (destinationSystem.isNull()) {
    return;
  }

  // ################################################
  // transform all observations to destination system
  // ################################################

  this->transformAllObsToDestSystem(destinationSystem);

  // ###################
  // recalc all features
  // ###################

  this->recalcFeatureSetWithoutTransformation();

  // #############################################################################
  // set nominals to solved only if their nominal system is the destination
  // system
  // #############################################################################

  // run through all nominal systems
  foreach (const oi::Guarded<CoordinateSystem>& system,
           this->currentJob->getCoordinateSystemsList()) {
    // check system
    if (system.isNull()) {
      continue;
    }

    // run through all nominals of the system
    foreach (const oi::Guarded<FeatureWrapper>& feature,
             system->getNominals()) {
      bool isSolved = (system == destinationSystem);

      // check feature
      if (!feature.isNull() && !feature->getGeometry().isNull()) {
        feature->getGeometry()->setIsSolved(isSolved);
      }
    }
  }
}

/*!
 * \brief FeatureUpdater::recalcFeatureSetWithoutTransformation
 */
void FeatureUpdater::recalcFeatureSetWithoutTransformation() {
  // check job
  if (this->currentJob.isNull()) {
    return;
  }

  // set all features to not have been updated before
  // recalcFeatureWithoutTransformation() - except function-owned
  // sub-geometry; see recalcFeatureSet()'s comment
  foreach (const oi::Guarded<FeatureWrapper>& feature,
           this->currentJob->getFeaturesList()) {
    if (!feature.isNull() && !feature->getFeature().isNull()) {
      if (!feature->getGeometry().isNull() &&
          feature->getGeometry()->getOwnerFeature().has_value()) {
        continue;
      }
      feature->getFeature()->blockSignals(true);
      feature->getFeature()->setIsUpdated(false);
      feature->getFeature()->blockSignals(false);
    }
  }

  // recalc recursively
  foreach (const oi::Guarded<FeatureWrapper>& feature,
           this->currentJob->getFeaturesList()) {
    // check feature
    if (feature.isNull() || feature->getFeature().isNull()) {
      continue;
    }

    // recalc the feature if it was not recalced yet and is no trafo param
    if (!feature->getFeature()->getIsUpdated() &&
        feature->getFeatureTypeEnum() != FeatureTypes::eTrafoParamFeature) {
      this->recalcFeatureWithoutTransformation(feature->getFeature());
    }
  }
}

/*!
 * \brief FeatureUpdater::recalcFeatureWithoutTransformation
 * \param feature
 */
void FeatureUpdater::recalcFeatureWithoutTransformation(
    const oi::Guarded<Feature>& feature) {
  // check job
  if (this->currentJob.isNull()) {
    return;
  }

  // check the feature
  if (feature.isNull() || feature->getFeatureWrapper().isNull() ||
      !feature->getFeatureWrapper()->getTrafoParam().isNull()) {
    return;
  }

  // set feature to not updated
  feature->setIsUpdated(false);

  // recalculate feature
  this->recursiveFeatureRecalculation(feature);
}

/*!
 * \brief FeatureUpdater::switchCoordinateSystemWithTransformation
 * \param destinationSystem
 */
void FeatureUpdater::switchCoordinateSystemWithTransformation(
    const oi::Guarded<CoordinateSystem>& destinationSystem) {
  // check current job
  if (this->currentJob.isNull()) {
    return;
  }

  // check destination system
  if (destinationSystem.isNull()) {
    return;
  }

  this->transformObsAndNominals(destinationSystem);

  // this->recalcFeatureSet();
}

/*!
 * \brief FeatureUpdater::transformObsAndNominals
 * \param destinationSystem
 */
void FeatureUpdater::transformObsAndNominals(
    const oi::Guarded<CoordinateSystem>& destinationSystem) {
  if (destinationSystem.isNull()) {
    return;  // nothing to do
  }
  // ################################################
  // transform all observations to destination system
  // ################################################
  qDebug() << "transformObsAndNominals: destinationSystem="
           << destinationSystem->getFeatureName();
  // run through all station systems
  foreach (const oi::Guarded<Station>& station,
           this->currentJob->getStationsList()) {
    // check station system
    if (station.isNull() || station->getCoordinateSystem().isNull()) {
      continue;
    }

    // transform all observations of the station system to the Destination
    // system
    this->trafoController.transformObservations(station->getCoordinateSystem(),
                                                destinationSystem);
    this->trafoController.transformCoordSystems(station->getCoordinateSystem(),
                                                destinationSystem, true);
  }

  this->recalcFeatureSet();

  // #############################################################################
  // set nominals to solved only if their nominal system is the destination
  // system
  // #############################################################################

  // run through all nominal systems
  foreach (const oi::Guarded<CoordinateSystem>& system,
           this->currentJob->getCoordinateSystemsList()) {
    // check system
    if (system.isNull()) {
      continue;
    }

    // run through all nominals of the system
    foreach (const oi::Guarded<FeatureWrapper>& feature,
             system->getNominals()) {
      bool isSolved = (system == destinationSystem);

      // check feature
      if (!feature.isNull() && !feature->getGeometry().isNull()) {
        feature->getFeature()->blockSignals(true);
        feature->getGeometry()->setIsSolved(isSolved);
        feature->getFeature()->blockSignals(false);
      }
    }

    // set origin value in current coordinate system
    this->trafoController.transformCoordSystems(system, destinationSystem,
                                                false);
  }
}

/*!
 * \brief FeatureUpdater::copyGeometry
 * \param newElement
 * \param oldElement
 * \param type
 */
void FeatureUpdater::copyGeometry(InputElement& newElement,
                                  const oi::Guarded<FeatureWrapper>& oldElement,
                                  const ElementTypes& type) {
  // add all elements
  switch (type) {
    case ElementTypes::eCircleElement: {
      if (!oldElement.isNull() && !oldElement->getCircle().isNull() &&
          !oldElement->getGeometry().isNull()) {
        newElement.circle = new Circle(*oldElement->getCircle().data());
        newElement.geometry = newElement.circle.data();
        newElement.typeOfElement = ElementTypes::eCircleElement;
      }
      return;
    }
    case ElementTypes::eConeElement: {
      if (!oldElement.isNull() && !oldElement->getCone().isNull() &&
          !oldElement->getGeometry().isNull()) {
        newElement.cone = new Cone(*oldElement->getCone().data());
        newElement.geometry = newElement.cone.data();
        newElement.typeOfElement = ElementTypes::eConeElement;
      }
      return;
    }
    case ElementTypes::eCylinderElement: {
      if (!oldElement.isNull() && !oldElement->getCylinder().isNull() &&
          !oldElement->getGeometry().isNull()) {
        newElement.cylinder = new Cylinder(*oldElement->getCylinder().data());
        newElement.geometry = newElement.cylinder.data();
        newElement.typeOfElement = ElementTypes::eCylinderElement;
      }
      return;
    }
    case ElementTypes::eEllipseElement: {
      if (!oldElement.isNull() && !oldElement->getEllipse().isNull() &&
          !oldElement->getGeometry().isNull()) {
        newElement.ellipse = new Ellipse(*oldElement->getEllipse().data());
        newElement.geometry = newElement.ellipse.data();
        newElement.typeOfElement = ElementTypes::eEllipseElement;
      }
      return;
    }
    case ElementTypes::eEllipsoidElement: {
      if (!oldElement.isNull() && !oldElement->getEllipsoid().isNull() &&
          !oldElement->getGeometry().isNull()) {
        newElement.ellipsoid =
            new Ellipsoid(*oldElement->getEllipsoid().data());
        newElement.geometry = newElement.ellipsoid.data();
        newElement.typeOfElement = ElementTypes::eEllipsoidElement;
      }
      return;
    }
    case ElementTypes::eHyperboloidElement: {
      if (!oldElement.isNull() && !oldElement->getHyperboloid().isNull() &&
          !oldElement->getGeometry().isNull()) {
        newElement.hyperboloid =
            new Hyperboloid(*oldElement->getHyperboloid().data());
        newElement.geometry = newElement.hyperboloid.data();
        newElement.typeOfElement = ElementTypes::eHyperboloidElement;
      }
      return;
    }
    case ElementTypes::eLineElement: {
      if (!oldElement.isNull() && !oldElement->getLine().isNull() &&
          !oldElement->getGeometry().isNull()) {
        newElement.line = new Line(*oldElement->getLine().data());
        newElement.geometry = newElement.line.data();
        newElement.typeOfElement = ElementTypes::eLineElement;
      }
      return;
    }
    case ElementTypes::eNurbsElement: {
      if (!oldElement.isNull() && !oldElement->getNurbs().isNull() &&
          !oldElement->getGeometry().isNull()) {
        newElement.nurbs = new Nurbs(*oldElement->getNurbs().data());
        newElement.geometry = newElement.nurbs.data();
        newElement.typeOfElement = ElementTypes::eNurbsElement;
      }
      return;
    }
    case ElementTypes::eParaboloidElement: {
      if (!oldElement.isNull() && !oldElement->getParaboloid().isNull() &&
          !oldElement->getGeometry().isNull()) {
        newElement.paraboloid =
            new Paraboloid(*oldElement->getParaboloid().data());
        newElement.geometry = newElement.paraboloid.data();
        newElement.typeOfElement = ElementTypes::eParaboloidElement;
      }
      return;
    }
    case ElementTypes::ePlaneElement: {
      if (!oldElement.isNull() && !oldElement->getPlane().isNull() &&
          !oldElement->getGeometry().isNull()) {
        newElement.plane = new Plane(*oldElement->getPlane().data());
        newElement.geometry = newElement.plane.data();
        newElement.typeOfElement = ElementTypes::ePlaneElement;
      }
      return;
    }
    case ElementTypes::ePointElement: {
      if (!oldElement.isNull() && !oldElement->getPoint().isNull() &&
          !oldElement->getGeometry().isNull()) {
        newElement.point = new Point(*oldElement->getPoint().data());
        newElement.geometry = newElement.point.data();
        newElement.typeOfElement = ElementTypes::ePointElement;
      }
      return;
    }
    case ElementTypes::ePointCloudElement: {
      if (!oldElement.isNull() && !oldElement->getPointCloud().isNull() &&
          !oldElement->getGeometry().isNull()) {
        newElement.pointCloud =
            new PointCloud(*oldElement->getPointCloud().data());
        newElement.geometry = newElement.pointCloud.data();
        newElement.typeOfElement = ElementTypes::ePointCloudElement;
      }
      return;
    }
    case ElementTypes::eScalarEntityAngleElement: {
      if (!oldElement.isNull() &&
          !oldElement->getScalarEntityAngle().isNull() &&
          !oldElement->getGeometry().isNull()) {
        newElement.scalarEntityAngle =
            new ScalarEntityAngle(*oldElement->getScalarEntityAngle().data());
        newElement.geometry = newElement.scalarEntityAngle.data();
        newElement.typeOfElement = ElementTypes::eScalarEntityAngleElement;
      }
      return;
    }
    case ElementTypes::eScalarEntityDistanceElement: {
      if (!oldElement.isNull() &&
          !oldElement->getScalarEntityDistance().isNull() &&
          !oldElement->getGeometry().isNull()) {
        newElement.scalarEntityDistance = new ScalarEntityDistance(
            *oldElement->getScalarEntityDistance().data());
        newElement.geometry = newElement.scalarEntityDistance.data();
        newElement.typeOfElement = ElementTypes::eScalarEntityDistanceElement;
      }
      return;
    }
    case ElementTypes::eScalarEntityMeasurementSeriesElement: {
      if (!oldElement.isNull() &&
          !oldElement->getScalarEntityMeasurementSeries().isNull() &&
          !oldElement->getGeometry().isNull()) {
        newElement.scalarEntityMeasurementSeries =
            new ScalarEntityMeasurementSeries(
                *oldElement->getScalarEntityMeasurementSeries().data());
        newElement.geometry = newElement.scalarEntityMeasurementSeries.data();
        newElement.typeOfElement =
            ElementTypes::eScalarEntityMeasurementSeriesElement;
      }
      return;
    }
    case ElementTypes::eScalarEntityTemperatureElement: {
      if (!oldElement.isNull() &&
          !oldElement->getScalarEntityTemperature().isNull() &&
          !oldElement->getGeometry().isNull()) {
        newElement.scalarEntityTemperature = new ScalarEntityTemperature(
            *oldElement->getScalarEntityTemperature().data());
        newElement.geometry = newElement.scalarEntityTemperature.data();
        newElement.typeOfElement =
            ElementTypes::eScalarEntityTemperatureElement;
      }
      return;
    }
    case ElementTypes::eSlottedHoleElement: {
      if (!oldElement.isNull() && !oldElement->getSlottedHole().isNull() &&
          !oldElement->getGeometry().isNull()) {
        newElement.slottedHole =
            new SlottedHole(*oldElement->getSlottedHole().data());
        newElement.geometry = newElement.slottedHole.data();
        newElement.typeOfElement = ElementTypes::eSlottedHoleElement;
      }
      return;
    }
    case ElementTypes::eSphereElement: {
      if (!oldElement.isNull() && !oldElement->getSphere().isNull() &&
          !oldElement->getGeometry().isNull()) {
        newElement.sphere = new Sphere(*oldElement->getSphere().data());
        newElement.geometry = newElement.sphere.data();
        newElement.typeOfElement = ElementTypes::eSphereElement;
      }
      return;
    }
    case ElementTypes::eTorusElement: {
      if (!oldElement.isNull() && !oldElement->getTorus().isNull() &&
          !oldElement->getGeometry().isNull()) {
        newElement.torus = new Torus(*oldElement->getTorus().data());
        newElement.geometry = newElement.torus.data();
        newElement.typeOfElement = ElementTypes::eTorusElement;
      }
      return;
    }
    case ElementTypes::eDirectionElement: {
      if (!oldElement.isNull() && !oldElement->getGeometry().isNull()) {
        this->copyGeometry(
            newElement, oldElement,
            getElementTypeEnum(oldElement->getFeatureTypeEnum()));
      }
      return;
    }
    case ElementTypes::ePositionElement: {
      if (!oldElement.isNull() && !oldElement->getGeometry().isNull()) {
        this->copyGeometry(
            newElement, oldElement,
            getElementTypeEnum(oldElement->getFeatureTypeEnum()));
      }
      return;
    }
    case ElementTypes::eRadiusElement: {
      if (!oldElement.isNull() && !oldElement->getGeometry().isNull()) {
        this->copyGeometry(
            newElement, oldElement,
            getElementTypeEnum(oldElement->getFeatureTypeEnum()));
      }
      return;
    }
  }
}

/*!
 * \brief FeatureUpdater::clearBundleResults
 * \param bundleSystem
 */
void FeatureUpdater::clearBundleResults(
    const oi::Guarded<CoordinateSystem>& bundleSystem) {
  // get and delete nominals in bundle system
  foreach (const oi::Guarded<FeatureWrapper> nominal,
           bundleSystem->getNominals()) {
    if (!nominal.isNull() && !nominal->getGeometry().isNull()) {
      delete nominal->getGeometry().data();
      delete nominal.data();
    }
  }

  // get and delete bundle transformations
  std::vector<oi::Guarded<TrafoParam> > transformations =
      bundleSystem->getTransformationParameters();
  foreach (const oi::Guarded<TrafoParam>& trafo, transformations) {
    if (!trafo.isNull() && trafo->getIsBundle()) {
      qDebug() << "FeatureUpdater::clearBundleResults transformation "
               << trafo->getId();
      // check if trafo is between a "bundle-station" and "bundle" or "bundle"
      // and "PART" system
      if (trafo->getStartSystem() == bundleSystem) {
        // if second coordinate system is a bundlestation => delete trafoParam
        if (!trafo->getDestinationSystem()->getStation().isNull()) {
          foreach (BundleStation bStation,
                   bundleSystem->getBundlePlugin()->getInputStations()) {
            if (bStation.id ==
                trafo->getDestinationSystem()->getStation()->getId()) {
              delete trafo.data();
              break;
            }
          }
        }
      } else if (trafo->getDestinationSystem() == bundleSystem) {
        if (!trafo->getStartSystem()->getStation().isNull()) {
          // if second coordinate system is a bundlestation => delete trafoParam
          foreach (BundleStation bStation,
                   bundleSystem->getBundlePlugin()->getInputStations()) {
            if (bStation.id == trafo->getStartSystem()->getStation()->getId()) {
              delete trafo.data();
              break;
            }
          }
        }
      }
    }
  }
}

/*!
 * \brief FeatureUpdater::saveBundleResults
 * \param bundleSystem
 */
void FeatureUpdater::saveBundleResults(
    const oi::Guarded<CoordinateSystem>& bundleSystem) {
  // get bundle plugin
  oi::Guarded<BundleAdjustment> plugin = bundleSystem->getBundlePlugin();

  // get bundle results
  std::vector<BundleGeometry> geometries = plugin->getOutputGeometries();
  std::vector<BundleTransformation> transformations =
      plugin->getOutputTransformations();

  // create nominal geometries in bundle system
  // this->createBundleGeometries(geometries, bundleSystem);

  // create bundle transformations
  this->createBundleTransformations(transformations, bundleSystem);
}

/*!
 * \brief FeatureUpdater::createBundleGeometries
 * \param geometries
 * \param bundleSystem
 */
/*void FeatureUpdater::createBundleGeometries(const std::vector<BundleGeometry>
&geometries, const oi::Guarded<CoordinateSystem> &bundleSystem){

    //create common points as nominals in bundle system
    std::vector<oi::Guarded<FeatureWrapper> > features;
    foreach(const BundleGeometry &geom, geometries){

        //get corresponding actual
        oi::Guarded<FeatureWrapper> actual =
this->currentJob->getFeatureById(geom.id); if(actual.isNull() ||
actual->getPoint().isNull()){ continue;
        }
        QString name = actual->getPoint()->getFeatureName();
        QString group = actual->getPoint()->getGroupName();

        //create point
        oi::Guarded<FeatureWrapper> feature = new FeatureWrapper();
        oi::Guarded<Point> point = new Point();
        feature->setPoint(point);

        //set up point attributes
        point->setNominalSystem(bundleSystem);
        point->setFeatureName(name);
        point->setGroupName(group);

        //set up point coordinates
        Position pos;
        double x, y, z;
        x = geom.parameters[eUnknownX];
        y = geom.parameters[eUnknownY];
        z = geom.parameters[eUnknownZ];
        pos.setVector(x, y, z);
        point->setPoint(pos);

        //add feature
        features.append(feature);

    }
    this->currentJob->addFeatures(features);

}*/

/*!
 * \brief FeatureUpdater::createBundleTransformations
 * \param transformations
 * \param bundleSystem
 */
void FeatureUpdater::createBundleTransformations(
    std::vector<BundleTransformation>& transformations,
    const oi::Guarded<CoordinateSystem>& bundleSystem) {
  // create transformations
  FeatureAttributes fAttr;
  fAttr.typeOfFeature = FeatureTypes::eTrafoParamFeature;
  fAttr.group = bundleSystem->getGroupName();
  fAttr.count = 1;
  fAttr.startSystem = bundleSystem->getFeatureName();
  std::vector<oi::Guarded<FeatureWrapper> > features;
  foreach (const BundleTransformation& transformation, transformations) {
    qDebug() << "transformation.id=" << transformation.id;

    // get station
    oi::Guarded<FeatureWrapper> feature =
        this->currentJob->getFeatureById(transformation.id);
    if (feature.isNull() || feature->getStation().isNull()) {
      continue;
    }
    QString stationName = feature->getStation()->getFeatureName();

    qDebug() << "feature->getStation()->getFeatureName() " << stationName;

    // set up name
    fAttr.name = bundleSystem->getFeatureName();
    int index = 1;
    while (!this->currentJob->validateFeatureName(fAttr.name,
                                                  fAttr.typeOfFeature)) {
      fAttr.name =
          QString("%1_%2").arg(bundleSystem->getFeatureName()).arg(index);
      index++;
    }

    // create trafo param feature
    fAttr.destinationSystem = stationName;
    oi::appendAll(features, this->currentJob->addFeatures(fAttr));
  }

  // check number of created transformations
  if (transformations.size() != features.size()) {
    this->currentJob->removeFeatures(features);
    return;
  }

  // set up transformation parameters
  int index = 0;
  foreach (const oi::Guarded<FeatureWrapper>& feature, features) {
    // check feature
    if (feature.isNull() || feature->getTrafoParam().isNull()) {
      index++;
      continue;
    }
    oi::Guarded<TrafoParam> trafo = feature->getTrafoParam();

    // set up parameters
    std::map<TrafoParamParameters, double> parameters =
        transformations.at(index).parameters;
    trafo->setUnknownParameters(parameters);
    trafo->setIsUsed(true);
    trafo->setIsDatumTrafo(true);

    index++;
  }
}

/*!
 * \brief FeatureUpdater::recalcAllBundleTrafoDependencies
 * \param bundleSystem
 */
void FeatureUpdater::recalcAllBundleTrafoDependencies(
    const oi::Guarded<CoordinateSystem>& bundleSystem) {
  // if bundle system
  if (bundleSystem->getIsBundleSystem()) {
    // get all trafo params from job
    std::vector<oi::Guarded<TrafoParam> > globalTps =
        this->currentJob->getTransformationParametersList();

    foreach (oi::Guarded<TrafoParam> gTP, globalTps) {
      bool recalc = true;

      if (gTP->getStartSystem() == bundleSystem ||
          gTP->getDestinationSystem() == bundleSystem) {
        // and check if they are included in bundle trafo
        foreach (oi::Guarded<TrafoParam> bundleTP,
                 bundleSystem->getTransformationParameters()) {
          if (gTP->getId() == bundleTP->getId()) {
            recalc = false;
          }
        }
      } else {
        recalc = false;
      }

      // if not, but they include the bundle system => recalc
      if (recalc) {
        gTP->recalc();
      }
    }
  }
}

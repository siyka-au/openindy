#include "projectexchanger.h"

#include "featureutil.h"
#include "version.h"

ProjectExchanger::ProjectExchanger() {}

ProjectExchanger::~ProjectExchanger() {}

/*!
 * \brief ProjectExchanger::saveProject create XML-Document, "saves" the project
 * / job to XML and compute a digest
 * \param job
 * \return
 */
QDomDocument ProjectExchanger::saveProject(const QPointer<OiJob>& job) {
  QDomDocument project;

  // check job
  if (job.isNull()) {
    Console::getInstance()->addLine("No project available",
                                    MessageTypes::eErrorMessage);
    return project;
  }

  // get current date and time
  QString dateTime = QDateTime::currentDateTime().toString(Qt::ISODate);

  // create XML root, set general project attributes later
  QDomElement root = project.createElement("oiProjectData");
  project.appendChild(root);

  // add active station and active coordinate system
  QDomElement activeSystem = project.createElement("activeCoordinateSystem");
  QDomElement activeStation = project.createElement("activeStation");
  int activeSystemId = 0, activeStationId = -1;
  if (!job->getActiveCoordinateSystem().isNull()) {
    activeSystemId = job->getActiveCoordinateSystem()->getId();
  }
  if (!job->getActiveStation().isNull()) {
    activeStationId = job->getActiveStation()->getId();
  }
  activeSystem.setAttribute("ref", activeSystemId);
  activeStation.setAttribute("ref", activeStationId);
  root.appendChild(activeSystem);
  root.appendChild(activeStation);

  // add stations
  QDomElement stations = project.createElement("stations");
  foreach (const QPointer<Station>& s, job->getStationsList()) {
    if (!s.isNull()) {
      QDomElement station = s->toOpenIndyXML(project);
      if (!station.isNull()) {
        stations.appendChild(station);
      }
    }
  }
  root.appendChild(stations);

  // add coordinate systems (nominal + station systems)
  QDomElement coordinateSystems = project.createElement("coordinateSystems");
  foreach (const QPointer<CoordinateSystem>& c,
           job->getCoordinateSystemsList()) {
    if (!c.isNull()) {
      QDomElement coordinateSystem = c->toOpenIndyXML(project);
      if (!coordinateSystem.isNull()) {
        coordinateSystems.appendChild(coordinateSystem);
      }
    }
  }
  foreach (const QPointer<Station>& s, job->getStationsList()) {
    if (!s.isNull() && !s->getCoordinateSystem().isNull()) {
      QDomElement coordinateSystem =
          s->getCoordinateSystem()->toOpenIndyXML(project);
      if (!coordinateSystem.isNull()) {
        coordinateSystems.appendChild(coordinateSystem);
      }
    }
  }
  root.appendChild(coordinateSystems);

  // add trafo params
  QDomElement trafoParams = project.createElement("transformationParameters");
  foreach (const QPointer<TrafoParam>& tp,
           job->getTransformationParametersList()) {
    if (!tp.isNull()) {
      QDomElement trafoParam = tp->toOpenIndyXML(project);
      if (!trafoParam.isNull()) {
        trafoParams.appendChild(trafoParam);
      }
    }
  }
  root.appendChild(trafoParams);

  // add geometries
  QDomElement geometries = project.createElement("geometries");
  foreach (const QPointer<FeatureWrapper>& geom,
           job->getGeometriesList()) {  // normal geometries
    if (!geom.isNull() && !geom->getGeometry().isNull()) {
      QDomElement geometry = geom->getGeometry()->toOpenIndyXML(project);
      if (!geometry.isNull()) {
        geometries.appendChild(geometry);
      }
    }
  }
  foreach (const QPointer<Station>& s,
           job->getStationsList()) {  // station points
    if (!s.isNull() && !s->getPosition().isNull()) {
      QDomElement geometry = s->getPosition()->toOpenIndyXML(project);
      if (!geometry.isNull()) {
        geometries.appendChild(geometry);
      }
    }
  }
  root.appendChild(geometries);

  // add groups (organisational only, Stage 7e - never a Geometry, so they
  // don't live in job->getGeometriesList() and need their own section)
  QDomElement groups = project.createElement("groups");
  foreach (const QPointer<FeatureWrapper>& g,
           job->getFeaturesByType(FeatureTypes::eGroupFeature)) {
    if (!g.isNull() && !g->getGroup().isNull()) {
      QDomElement group = g->getGroup()->toOpenIndyXML(project);
      if (!group.isNull()) {
        groups.appendChild(group);
      }
    }
  }
  foreach (const QPointer<FeatureWrapper>& g,
           job->getFeaturesByType(FeatureTypes::ePointGroupFeature)) {
    if (!g.isNull() && !g->getGroup().isNull()) {
      QDomElement group = g->getGroup()->toOpenIndyXML(project);
      if (!group.isNull()) {
        groups.appendChild(group);
      }
    }
  }
  root.appendChild(groups);

  // add observations (search the station coordinate systems for observations)
  QDomElement observations = project.createElement("observations");
  foreach (const QPointer<Station>& s, job->getStationsList()) {
    if (!s.isNull() && !s->getCoordinateSystem().isNull() &&
        s->getCoordinateSystem()->getObservations().size() > 0) {
      foreach (const QPointer<Observation>& obs,
               s->getCoordinateSystem()->getObservations()) {
        if (!obs.isNull()) {
          QDomElement observation = obs->toOpenIndyXML(project);
          if (!observation.isNull()) {
            observations.appendChild(observation);
          }
        }
      }
    }
  }
  root.appendChild(observations);

  // add configs

  // TODO only save necessary mConfigs

  QDomElement configs = project.createElement("configs");
  QDomElement measurementConfigs = project.createElement("measurementConfigs");
  // QDomElement sensorConfigs = project.createElement("sensorConfigs");

  // add measurement configs
  if (!this->mConfigManager.isNull()) {
    QSet<MeasurementConfigKey> mConfigs;
    foreach (const QPointer<FeatureWrapper>& feature,
             job->getGeometriesList()) {
      MeasurementConfigKey mConfig =
          feature->getGeometry()->getMeasurementConfig();
      if (mConfig.isValid()) {
        mConfigs.insert(mConfig);
      }
    }

    foreach (const MeasurementConfigKey& mConfig, mConfigs.values()) {
      if (mConfig.isStandardConfig()) {
        continue;
      }
      // used user or project configs
      MeasurementConfig mc =
          this->getMeasurementConfigManager()->getConfig(mConfig);
      if (mc.isValid()) {
        QDomElement config = mc.toOpenIndyXML(project);
        if (!config.isNull()) {
          measurementConfigs.appendChild(config);
        }
      }
    }
  }

  configs.appendChild(measurementConfigs);

  // add sensor configs
  /*QList<SensorConfiguration> sConfigs = OiConfigState::getAllSensorConfigs();
  foreach(const SensorConfiguration &sConfig, sConfigs){
      QDomElement config = sConfig.toOpenIndyXML(project);
      if(!config.isNull()){
          sensorConfigs.appendChild(config);
      }
  }
  configs.appendChild(sensorConfigs);*/

  root.appendChild(configs);

  // create hash / digest over xml with out oiProjectData Attributes
  QByteArray arr;
  QTextStream stream(&arr);
  project.save(stream, 4 /*indent*/);
  stream.flush();
  QCryptographicHash hash(QCryptographicHash::Sha256);
  hash.addData(arr);
  QString digest = hash.result().toHex();

  // set hash / digest
  job->setDigest(digest);

  // set general project attributes
  root.setAttribute("name", job->getJobName());
  root.setAttribute("date", dateTime);
  root.setAttribute("idCount", QString::number(job->generateUniqueId()));
  root.setAttribute("version", QString(OPENINDY_VERSION));
  root.setAttribute("digest", digest);

  return project;
}

/*!
 * \brief ProjectExchanger::loadProject
 * \param project
 * \return
 */
const QPointer<OiJob> ProjectExchanger::loadProject(
    const QDomDocument& project) {
  QPointer<OiJob> job = new OiJob();

  // load all elements from xml into helper lists
  if (!this->loadObservations(project) || !this->loadStations(project) ||
      !this->loadCoordinateSystems(project) ||
      !this->loadTransformationParameters(project) ||
      !this->loadGeometries(project) || !this->loadGroups(project) ||
      !this->loadConfigs(project)) {
    // clear all created elements if an error occured in one of the loading
    // helpers (e.g. no station available)
    this->clearHelperMaps(true);
    return nullptr;
  }

  // restore dependencies between the elements
  if (!this->restoreObservationDependencies(project) ||
      !this->restoreStationDependencies(project) ||
      !this->restoreCoordinateSystemDependencies(project) ||
      !this->restoreTrafoParamDependencies(project) ||
      !this->restoreGeometryDependencies(project) ||
      !this->restoreGroupDependencies(project)) {
    // clear all created elements if an error occured in one of the loading
    // helpers (e.g. no station available)
    this->clearHelperMaps(true);
    return nullptr;
  }

  // remove station points
  foreach (const QPointer<FeatureWrapper>& station, this->myStations) {
    if (!station.isNull() && !station->getStation().isNull()) {
      if (!station->getStation()->getPosition().isNull()) {
        this->myGeometries.remove(
            station->getStation()->getPosition()->getId());
      }
    }
  }

  // add features to the job
  job->addFeaturesFromXml(this->myStations.values());
  job->addFeaturesFromXml(this->myCoordinateSystems.values());
  job->addFeaturesFromXml(this->myTransformationParameters.values());
  job->addFeaturesFromXml(this->myGeometries.values());
  job->addFeaturesFromXml(this->myGroups.values());

  // set id count
  if (project.documentElement().hasAttribute("idCount")) {
    job->nextId = project.documentElement().attribute("idCount").toInt() + 1;
  }

  // create Bundle01_1 if not exists
  if (job->getFeaturesByName("Bundle01_1").isEmpty()) {
    FeatureUtil util;
    if (!util.createInitialBundleTransformation(job)) {
      this->clearHelperMaps(true);
      return nullptr;
    }
  }

  if (project.documentElement().hasAttribute("digest")) {
    job->setDigest(project.documentElement().attribute("digest"));
  } else {  // not available, then compute
    // compute digest and store digest in job
    this->saveProject(job);
  }

  if (project.documentElement().hasAttribute("version")) {
    job->setLoadedProjectVersion(
        project.documentElement().attribute("version"));
  }

  // add project measurement configs to config manager
  if (!this->mConfigManager.isNull()) {
    // remove project config loaded from previous job
    foreach (const MeasurementConfig& mConfig,
             this->mConfigManager->getProjectConfigs()) {
      this->mConfigManager->removeProjectConfig(mConfig.getName());
    }

    // add project config loaded from this job if is not an standard config
    foreach (const MeasurementConfig& mConfig,
             this->measurementConfigs.values()) {
      if (mConfig.isValid() && mConfig.isProjectConfig() &&
          !this->mConfigManager->getStandardConfig(mConfig.getName())
               .isValid()) {
        this->mConfigManager->addProjectConfig(mConfig);
      }
    }
  }

  // set active station and active coordinate system
  QDomElement activeStation =
      project.documentElement().firstChildElement("activeStation");
  QDomElement activeCoordinateSystem =
      project.documentElement().firstChildElement("activeCoordinateSystem");
  if (activeStation.isNull() || activeCoordinateSystem.isNull() ||
      !activeStation.hasAttribute("ref") ||
      !activeCoordinateSystem.hasAttribute("ref") ||
      !this->myStations.contains(activeStation.attribute("ref").toInt()) ||
      !this->myCoordinateSystems.contains(
          activeCoordinateSystem.attribute("ref").toInt())) {
    // clear all created elements if no active station or active coordinate
    // system is available
    this->clearHelperMaps(true);
    return nullptr;
  }
  QPointer<FeatureWrapper> myActiveStation =
      this->myStations.value(activeStation.attribute("ref").toInt());
  QPointer<FeatureWrapper> myActiveSystem = this->myCoordinateSystems.value(
      activeCoordinateSystem.attribute("ref").toInt());
  myActiveStation->getStation()->setActiveStationState(true);
  myActiveSystem->getCoordinateSystem()->setActiveCoordinateSystemState(true);

  // clear the helper maps
  this->clearHelperMaps(false);

  return job;
}

/*!
 * \brief ProjectExchanger::getMeasurementConfigManager
 * \return
 */
QPointer<MeasurementConfigManager>&
ProjectExchanger::getMeasurementConfigManager() {
  return this->mConfigManager;
}

/*!
 * \brief ProjectExchanger::setMeasurementConfigManager
 * \param mConfigManager
 */
void ProjectExchanger::setMeasurementConfigManager(
    const QPointer<MeasurementConfigManager>& mConfigManager) {
  this->mConfigManager = mConfigManager;
}

/*!
 * \brief ProjectExchanger::loadObservations
 * \param project
 * \return
 */
bool ProjectExchanger::loadObservations(const QDomDocument& project) {
  QDomElement observations =
      project.documentElement().firstChildElement("observations");
  if (!observations.isNull()) {
    // get a list of all observations tags
    QDomNodeList observationList = observations.childNodes();

    for (int i = 0; i < observationList.size(); i++) {
      // for each observations tag create an observation object and try to parse
      // xml
      QPointer<Observation> obs = new Observation();
      QDomElement obsTag = observationList.at(i).toElement();
      if (!obs->fromOpenIndyXML(obsTag)) {
        delete obs;
        continue;
      }

      // add the observation to the list of successfully loaded observations
      this->myObservations.insert(obs->getId(), obs);
      this->myReadings.insert(obs->getReading()->getId(), obs->getReading());
    }
  }

  return true;
}

/*!
 * \brief ProjectExchanger::loadStations
 * \param project
 * \return
 */
bool ProjectExchanger::loadStations(const QDomDocument& project) {
  QDomElement stations =
      project.documentElement().firstChildElement("stations");

  // at least one station has to be available
  if (stations.isNull()) {
    return false;
  }

  // get a list of all station tags
  QDomNodeList stationList = stations.childNodes();

  for (int i = 0; i < stationList.size(); i++) {
    // for each station tag create a station object and try to parse xml
    QPointer<Station> station = new Station();
    QDomElement stationTag = stationList.at(i).toElement();
    if (!station->fromOpenIndyXML(stationTag)) {
      delete station;
      continue;
    }

    // create a feature wrapper for the station
    QPointer<FeatureWrapper> stationWrapper = new FeatureWrapper();
    stationWrapper->setStation(station);

    // add the station to the list of successfully loaded stations
    this->myStations.insert(station->getId(), stationWrapper);
  }

  // at least one station has to be available
  if (this->myStations.size() == 0) {
    return false;
  }

  return true;
}

/*!
 * \brief ProjectExchanger::loadCoordinateSystems
 * \param project
 * \return
 */
bool ProjectExchanger::loadCoordinateSystems(const QDomDocument& project) {
  QDomElement coordinateSystems =
      project.documentElement().firstChildElement("coordinateSystems");

  // at least one coordinate system has to be available
  if (coordinateSystems.isNull()) {
    return false;
  }

  // get a list of all coordinate system tags
  QDomNodeList coordinateSystemList = coordinateSystems.childNodes();

  for (int i = 0; i < coordinateSystemList.size(); i++) {
    // for each coordinate system tag create a coordinate system object and try
    // to parse xml
    QPointer<CoordinateSystem> coordinateSystem = new CoordinateSystem();
    QDomElement coordTag = coordinateSystemList.at(i).toElement();
    if (!coordinateSystem->fromOpenIndyXML(coordTag)) {
      delete coordinateSystem;
      continue;
    }

    // create a feature wrapper for the station
    QPointer<FeatureWrapper> coordinateSystemWrapper = new FeatureWrapper();
    coordinateSystemWrapper->setCoordinateSystem(coordinateSystem);

    // add the coordinate system to the list of successfully loaded coordinate
    // systems
    this->myCoordinateSystems.insert(coordinateSystem->getId(),
                                     coordinateSystemWrapper);
  }

  // at least one coordinate system has to be available
  if (this->myCoordinateSystems.size() == 0) {
    return false;
  }

  return true;
}

/*!
 * \brief ProjectExchanger::loadTransformationParameters
 * \param project
 * \return
 */
bool ProjectExchanger::loadTransformationParameters(
    const QDomDocument& project) {
  QDomElement trafoParams =
      project.documentElement().firstChildElement("transformationParameters");
  if (!trafoParams.isNull()) {
    // get a list of all trafo param tags
    QDomNodeList trafoParamList = trafoParams.childNodes();

    for (int i = 0; i < trafoParamList.size(); i++) {
      // for each trafo param tag create a trafo param object and try to parse
      // xml
      QPointer<TrafoParam> trafoParam = new TrafoParam();
      QDomElement trafoTag = trafoParamList.at(i).toElement();
      if (!trafoParam->fromOpenIndyXML(trafoTag)) {
        delete trafoParam;
        continue;
      }

      // create a feature wrapper for the station
      QPointer<FeatureWrapper> trafoParamWrapper = new FeatureWrapper();
      trafoParamWrapper->setTrafoParam(trafoParam);

      // add the trafo param to the list of successfully loaded trafo params
      this->myTransformationParameters.insert(trafoParam->getId(),
                                              trafoParamWrapper);
    }
  }

  return true;
}

/*!
 * \brief ProjectExchanger::loadGeometries
 * \param project
 * \return
 */
bool ProjectExchanger::loadGeometries(const QDomDocument& project) {
  QDomElement geometries =
      project.documentElement().firstChildElement("geometries");
  if (!geometries.isNull()) {
    // get a list of all geometry tags
    QDomNodeList geometryList = geometries.childNodes();

    for (int i = 0; i < geometryList.size(); i++) {
      // get the geometry tag and check if the type attribute is available
      QDomElement geometry = geometryList.at(i).toElement();
      if (geometry.isNull() || !geometry.hasAttribute("type")) {
        continue;
      }

      // get the type of geometry
      FeatureTypes geomType = getFeatureTypeEnum(geometry.attribute("type"));

      // create a feature wrapper object to hold the geometry
      QPointer<FeatureWrapper> myFeatureWrapper;
      myFeatureWrapper = new FeatureWrapper();

      // depending on the type of geometry call the right fromOpenIndyXML method
      switch (geomType) {
        case FeatureTypes::eCircleFeature: {
          QPointer<Circle> myCircle = new Circle();
          myFeatureWrapper->setCircle(myCircle);
          if (!myCircle->fromOpenIndyXML(geometry)) {
            delete myCircle;
            delete myFeatureWrapper;
            continue;
          }
          break;
        }
        case FeatureTypes::eConeFeature: {
          QPointer<Cone> myCone = new Cone();
          myFeatureWrapper->setCone(myCone);
          if (!myCone->fromOpenIndyXML(geometry)) {
            delete myCone;
            delete myFeatureWrapper;
            continue;
          }
          break;
        }
        case FeatureTypes::eCylinderFeature: {
          QPointer<Cylinder> myCylinder = new Cylinder();
          myFeatureWrapper->setCylinder(myCylinder);
          if (!myCylinder->fromOpenIndyXML(geometry)) {
            delete myCylinder;
            delete myFeatureWrapper;
            continue;
          }
          break;
        }
        case FeatureTypes::eEllipseFeature: {
          QPointer<Ellipse> myEllipse = new Ellipse();
          myFeatureWrapper->setEllipse(myEllipse);
          if (!myEllipse->fromOpenIndyXML(geometry)) {
            delete myEllipse;
            delete myFeatureWrapper;
            continue;
          }
          break;
        }
        case FeatureTypes::eEllipsoidFeature: {
          QPointer<Ellipsoid> myEllipsoid = new Ellipsoid();
          myFeatureWrapper->setEllipsoid(myEllipsoid);
          if (!myEllipsoid->fromOpenIndyXML(geometry)) {
            delete myEllipsoid;
            delete myFeatureWrapper;
            continue;
          }
          break;
        }
        case FeatureTypes::eHyperboloidFeature: {
          QPointer<Hyperboloid> myHyperboloid = new Hyperboloid();
          myFeatureWrapper->setHyperboloid(myHyperboloid);
          if (!myHyperboloid->fromOpenIndyXML(geometry)) {
            delete myHyperboloid;
            delete myFeatureWrapper;
            continue;
          }
          break;
        }
        case FeatureTypes::eLineFeature: {
          QPointer<Line> myLine = new Line();
          myFeatureWrapper->setLine(myLine);
          if (!myLine->fromOpenIndyXML(geometry)) {
            delete myLine;
            delete myFeatureWrapper;
            continue;
          }
          break;
        }
        case FeatureTypes::eNurbsFeature: {
          QPointer<Nurbs> myNurbs = new Nurbs();
          myFeatureWrapper->setNurbs(myNurbs);
          if (!myNurbs->fromOpenIndyXML(geometry)) {
            delete myNurbs;
            delete myFeatureWrapper;
            continue;
          }
          break;
        }
        case FeatureTypes::eParaboloidFeature: {
          QPointer<Paraboloid> myParaboloid = new Paraboloid();
          myFeatureWrapper->setParaboloid(myParaboloid);
          if (!myParaboloid->fromOpenIndyXML(geometry)) {
            delete myParaboloid;
            delete myFeatureWrapper;
            continue;
          }
          break;
        }
        case FeatureTypes::ePlaneFeature: {
          QPointer<Plane> myPlane = new Plane();
          myFeatureWrapper->setPlane(myPlane);
          if (!myPlane->fromOpenIndyXML(geometry)) {
            delete myPlane;
            delete myFeatureWrapper;
            continue;
          }
          break;
        }
        case FeatureTypes::ePointFeature: {
          QPointer<Point> myPoint = new Point();
          myFeatureWrapper->setPoint(myPoint);
          if (!myPoint->fromOpenIndyXML(geometry)) {
            delete myPoint;
            delete myFeatureWrapper;
            continue;
          }
          break;
        }
        case FeatureTypes::ePointCloudFeature: {
          QPointer<PointCloud> myPointCloud = new PointCloud();
          myFeatureWrapper->setPointCloud(myPointCloud);
          if (!myPointCloud->fromOpenIndyXML(geometry)) {
            delete myPointCloud;
            delete myFeatureWrapper;
            continue;
          }
          break;
        }
        case FeatureTypes::eScalarEntityAngleFeature: {
          QPointer<ScalarEntityAngle> myAngle = new ScalarEntityAngle();
          myFeatureWrapper->setScalarEntityAngle(myAngle);
          if (!myAngle->fromOpenIndyXML(geometry)) {
            delete myAngle;
            delete myFeatureWrapper;
            continue;
          }
          break;
        }
        case FeatureTypes::eScalarEntityDistanceFeature: {
          QPointer<ScalarEntityDistance> myDistance =
              new ScalarEntityDistance();
          myFeatureWrapper->setScalarEntityDistance(myDistance);
          if (!myDistance->fromOpenIndyXML(geometry)) {
            delete myDistance;
            delete myFeatureWrapper;
            continue;
          }
          break;
        }
        case FeatureTypes::eScalarEntityMeasurementSeriesFeature: {
          QPointer<ScalarEntityMeasurementSeries> myMeasurementSeries =
              new ScalarEntityMeasurementSeries();
          myFeatureWrapper->setScalarEntityMeasurementSeries(
              myMeasurementSeries);
          if (!myMeasurementSeries->fromOpenIndyXML(geometry)) {
            delete myMeasurementSeries;
            delete myFeatureWrapper;
            continue;
          }
          break;
        }
        case FeatureTypes::eScalarEntityTemperatureFeature: {
          QPointer<ScalarEntityTemperature> myTemperature =
              new ScalarEntityTemperature();
          myFeatureWrapper->setScalarEntityTemperature(myTemperature);
          if (!myTemperature->fromOpenIndyXML(geometry)) {
            delete myTemperature;
            delete myFeatureWrapper;
            continue;
          }
          break;
        }
        case FeatureTypes::eSlottedHoleFeature: {
          QPointer<SlottedHole> mySlottedHole = new SlottedHole();
          myFeatureWrapper->setSlottedHole(mySlottedHole);
          if (!mySlottedHole->fromOpenIndyXML(geometry)) {
            delete mySlottedHole;
            delete myFeatureWrapper;
            continue;
          }
          break;
        }
        case FeatureTypes::eSphereFeature: {
          QPointer<Sphere> mySphere = new Sphere();
          myFeatureWrapper->setSphere(mySphere);
          if (!mySphere->fromOpenIndyXML(geometry)) {
            delete mySphere;
            delete myFeatureWrapper;
            continue;
          }
          break;
        }
        case FeatureTypes::eTorusFeature: {
          QPointer<Torus> myTorus = new Torus();
          myFeatureWrapper->setTorus(myTorus);
          if (!myTorus->fromOpenIndyXML(geometry)) {
            delete myTorus;
            delete myFeatureWrapper;
            continue;
          }
          break;
        }
        default: {
          delete myFeatureWrapper;
          continue;
        }
      }

      // add the feature wrapper to the list of successfully loaded geometries
      this->myGeometries.insert(myFeatureWrapper->getFeature()->getId(),
                                myFeatureWrapper);
    }
  }

  return true;
}

/*!
 * \brief ProjectExchanger::loadGroups
 * Group/PointGroup are organisational Features, not Geometry, so they get
 * their own XML section and load pass rather than sharing loadGeometries's
 * switch (Stage 7e). Membership itself is not resolved here - a member may
 * be a feature not loaded yet, so that happens in restoreGroupDependencies
 * once every feature in the project exists, the same two-pass shape
 * restoreGeometryDependencies already uses for realises/ownerFeature.
 * \param project
 * \return
 */
bool ProjectExchanger::loadGroups(const QDomDocument& project) {
  QDomElement groups = project.documentElement().firstChildElement("groups");
  if (!groups.isNull()) {
    QDomNodeList groupList = groups.childNodes();

    for (int i = 0; i < groupList.size(); i++) {
      QDomElement group = groupList.at(i).toElement();
      if (group.isNull() || !group.hasAttribute("type")) {
        continue;
      }

      const FeatureTypes groupType =
          getFeatureTypeEnum(group.attribute("type"));

      QPointer<FeatureWrapper> myFeatureWrapper = new FeatureWrapper();

      switch (groupType) {
        case FeatureTypes::eGroupFeature: {
          QPointer<Group> myGroup = new Group();
          myFeatureWrapper->setGroup(myGroup);
          if (!myGroup->fromOpenIndyXML(group)) {
            delete myGroup;
            delete myFeatureWrapper;
            continue;
          }
          break;
        }
        case FeatureTypes::ePointGroupFeature: {
          QPointer<PointGroup> myPointGroup = new PointGroup();
          myFeatureWrapper->setPointGroup(myPointGroup);
          if (!myPointGroup->fromOpenIndyXML(group)) {
            delete myPointGroup;
            delete myFeatureWrapper;
            continue;
          }
          break;
        }
        default: {
          delete myFeatureWrapper;
          continue;
        }
      }

      // add the feature wrapper to the list of successfully loaded groups
      this->myGroups.insert(myFeatureWrapper->getFeature()->getId(),
                            myFeatureWrapper);
    }
  }

  return true;
}

/*!
 * \brief ProjectExchanger::loadConfigs
 * \param project
 * \return
 */
bool ProjectExchanger::loadConfigs(const QDomDocument& project) {
  QDomElement configs = project.documentElement().firstChildElement("configs");

  if (!configs.isNull()) {
    // measurement configs
    QDomElement mConfigs = configs.firstChildElement("measurementConfigs");
    if (!mConfigs.isNull()) {
      QDomNodeList mConfigList = mConfigs.childNodes();
      for (int i = 0; i < mConfigList.size(); ++i) {
        QDomElement mConfigElement = mConfigList.at(i).toElement();
        MeasurementConfig mConfig;
        if (mConfig.fromOpenIndyXML(mConfigElement)) {
          mConfig.makeProjectConfig();
          this->measurementConfigs.insert(mConfig.getKey(), mConfig);
        }
      }
    }

    // sensor configs
    QDomElement sConfigs = configs.firstChildElement("sensorConfigs");
    if (!sConfigs.isNull()) {
      QDomNodeList sConfigList = sConfigs.childNodes();
      for (int i = 0; i < sConfigList.size(); ++i) {
        QDomElement sConfigElement = sConfigList.at(i).toElement();
        SensorConfiguration sConfig;
        if (sConfig.fromOpenIndyXML(sConfigElement)) {
          this->mySConfigs.insert(sConfig.getName(), sConfig);
        }
      }
    }
  }

  return true;
}

/*!
 * \brief ProjectExchanger::restoreStationDependencies
 * \param project
 * \return
 */
bool ProjectExchanger::restoreStationDependencies(const QDomDocument& project) {
  bool result = true;

  // iterate through the station tags in the xml project
  QDomElement stations =
      project.documentElement().firstChildElement("stations");
  if (!stations.isNull()) {
    QDomNodeList stationList = stations.childNodes();
    for (int i = 0; i < stationList.size(); i++) {
      // get the station tag at position i and the corresponding FeatureWrapper
      QDomElement station = stationList.at(i).toElement();
      if (!station.hasAttribute("id") ||
          !this->myStations.contains(station.attribute("id").toInt())) {
        continue;
      }
      QPointer<FeatureWrapper> myStation =
          this->myStations.value(station.attribute("id").toInt());
      if (myStation.isNull() || myStation->getStation().isNull()) {
        continue;
      }

      result = true;

      // load used sensor plugins
      QDomElement usedSensors = station.firstChildElement("usedSensors");
      if (!usedSensors.isNull()) {
        QDomNodeList usedSensorsList = usedSensors.childNodes();
        QList<Sensor> myUsedSensors;
        for (int j = 0; j < usedSensorsList.size(); j++) {
          QDomElement usedSensor = usedSensorsList.at(j).toElement();
          if (usedSensor.hasAttribute("name") &&
              usedSensor.hasAttribute("plugin")) {
            std::optional<sys::Plugin> plugin =
                PluginRepository::getByName(usedSensor.attribute("plugin"));
            if (plugin.has_value()) {
              QPointer<Sensor> mySensor = PluginLoader::loadSensorPlugin(
                  plugin->filePath(), usedSensor.attribute("name"));
              if (!mySensor.isNull()) {
                myUsedSensors.append(Sensor(*mySensor.data()));
                delete mySensor;
              }
            }
          }
        }
        myStation->getStation()->setUsedSensors(myUsedSensors);
      }

      // load active sensor plugin
      QDomElement activeSensor = station.firstChildElement("activeSensor");
      if (!activeSensor.isNull() && activeSensor.hasAttribute("name") &&
          activeSensor.hasAttribute("plugin")) {
        std::optional<sys::Plugin> plugin =
            PluginRepository::getByName(activeSensor.attribute("plugin"));
        if (plugin.has_value()) {
          QPointer<Sensor> mySensor = PluginLoader::loadSensorPlugin(
              plugin->filePath(), activeSensor.attribute("name"));
          if (!mySensor.isNull()) {
            myStation->getStation()->setSensor(mySensor);
          }
        }
      }

      // set position point
      QDomElement position = station.firstChildElement("position");
      if (!position.isNull() && position.hasAttribute("ref") &&
          this->myGeometries.contains(position.attribute("ref").toInt())) {
        QPointer<FeatureWrapper> myPosition =
            this->myGeometries.value(position.attribute("ref").toInt());
        if (!myPosition.isNull() && !myPosition->getPoint().isNull()) {
          myStation->getStation()->position = myPosition->getPoint();
        }
      }

      // set coordinate system
      QDomElement coordinateSystem =
          station.firstChildElement("coordinateSystem");
      if (!coordinateSystem.isNull() && coordinateSystem.hasAttribute("ref")) {
        QPointer<FeatureWrapper> mySystem = this->myCoordinateSystems.value(
            coordinateSystem.attribute("ref").toInt());
        if (!mySystem.isNull() && !mySystem->getCoordinateSystem().isNull()) {
          myStation->getStation()->stationSystem =
              mySystem->getCoordinateSystem();
          myStation->getStation()->stationSystem->isStationSystem = true;
          mySystem->getCoordinateSystem()->station = myStation->getStation();
        } else {
          result = false;
        }
      } else {
        result = false;
      }

      // set used for
      QDomElement usedFor = station.firstChildElement("usedFor");
      if (!usedFor.isNull()) {
        QDomNodeList usedForList = usedFor.childNodes();
        for (int j = 0; j < usedForList.size(); j++) {
          QDomElement feature = usedForList.at(j).toElement();
          if (feature.hasAttribute("ref")) {
            QPointer<FeatureWrapper> myFeature(nullptr);
            if (this->myGeometries.contains(feature.attribute("ref").toInt())) {
              myFeature =
                  this->myGeometries.value(feature.attribute("ref").toInt());
            } else if (this->myCoordinateSystems.contains(
                           feature.attribute("ref").toInt())) {
              myFeature = this->myCoordinateSystems.value(
                  feature.attribute("ref").toInt());
            } else if (this->myTransformationParameters.contains(
                           feature.attribute("ref").toInt())) {
              myFeature = this->myTransformationParameters.value(
                  feature.attribute("ref").toInt());
            }
            if (!myFeature.isNull()) {
              myStation->getStation()->addUsedFor(myFeature);
            }
          }
        }
      }
    }
  }

  return result;
}

/*!
 * \brief ProjectExchanger::restoreCoordinateSystemDependencies
 * \param project
 * \return
 */
bool ProjectExchanger::restoreCoordinateSystemDependencies(
    const QDomDocument& project) {
  bool result = true;

  // iterate through the coordinate system tags in the xml project
  QDomElement coordinateSystems =
      project.documentElement().firstChildElement("coordinateSystems");
  if (!coordinateSystems.isNull()) {
    QDomNodeList coordinateSystemList = coordinateSystems.childNodes();
    for (int i = 0; i < coordinateSystemList.size(); i++) {
      // get the coordinate system tag at position i and the corresponding
      // FeatureWrapper
      QDomElement coordinateSystem = coordinateSystemList.at(i).toElement();
      if (!coordinateSystem.hasAttribute("id") ||
          !this->myCoordinateSystems.contains(
              coordinateSystem.attribute("id").toInt())) {
        continue;
      }
      QPointer<FeatureWrapper> myCoordinateSystem =
          this->myCoordinateSystems.value(
              coordinateSystem.attribute("id").toInt());
      if (myCoordinateSystem.isNull() ||
          myCoordinateSystem->getCoordinateSystem().isNull()) {
        continue;
      }

      result = true;

      // load function plugins
      QDomElement functions = coordinateSystem.firstChildElement("functions");
      if (!functions.isNull()) {
        QList<QPointer<Function> > myFunctions =
            this->restoreFunctionDependencies(functions);
        for (int j = 0; j < myFunctions.size(); j++) {
          myCoordinateSystem->getCoordinateSystem()->addFunction(
              myFunctions.at(j));
        }
      }

      // load bundle adjustments
      QDomElement bundle_plugin = coordinateSystem.firstChildElement("bundle");
      if (!bundle_plugin.isNull()) {
        QPointer<BundleAdjustment> myBundle =
            this->restoreBundleDependencies(bundle_plugin);
        if (!myBundle.isNull()) {
          myCoordinateSystem->getCoordinateSystem()->setBundlePlugin(myBundle);
        }
      }

      // set observations (made from a station system)
      QDomElement observations =
          coordinateSystem.firstChildElement("observations");
      if (!observations.isNull()) {
        QDomNodeList observationList = observations.childNodes();
        for (int j = 0; j < observationList.size(); j++) {
          QDomElement observation = observationList.at(j).toElement();
          if (observation.hasAttribute("ref") &&
              this->myObservations.contains(
                  observation.attribute("ref").toInt())) {
            QPointer<Observation> myObservation = this->myObservations.value(
                observation.attribute("ref").toInt());
            myCoordinateSystem->getCoordinateSystem()->observationsList.append(
                myObservation);
            myCoordinateSystem->getCoordinateSystem()->observationsMap.insert(
                myObservation->getId(), myObservation);
          }
        }
      }

      // set nominal geometries
      QDomElement nominalGeometries =
          coordinateSystem.firstChildElement("nominalGeometries");
      if (!nominalGeometries.isNull()) {
        QDomNodeList geometryList = nominalGeometries.childNodes();
        for (int j = 0; j < geometryList.size(); j++) {
          QDomElement geometry = geometryList.at(j).toElement();
          if (geometry.hasAttribute("ref") &&
              this->myGeometries.contains(geometry.attribute("ref").toInt())) {
            QPointer<FeatureWrapper> myGeometry =
                this->myGeometries.value(geometry.attribute("ref").toInt());
            if (!myGeometry->getGeometry()->getOwnerDescription().has_value()) {
              myCoordinateSystem->getCoordinateSystem()->nominalsList.append(
                  myGeometry);
              myCoordinateSystem->getCoordinateSystem()->nominalsMap.insert(
                  myGeometry->getGeometry()->getId(), myGeometry);
            }
          }
        }
      }

      // set used for
      QDomElement usedFor = coordinateSystem.firstChildElement("usedFor");
      if (!usedFor.isNull()) {
        QDomNodeList featureList = usedFor.childNodes();
        for (int j = 0; j < featureList.size(); j++) {
          QDomElement feature = featureList.at(j).toElement();
          if (feature.hasAttribute("ref")) {
            if (this->myGeometries.contains(feature.attribute("ref").toInt())) {
              myCoordinateSystem->getCoordinateSystem()->addUsedFor(
                  this->myGeometries.value(feature.attribute("ref").toInt()));
            } else if (this->myCoordinateSystems.contains(
                           feature.attribute("ref").toInt())) {
              myCoordinateSystem->getCoordinateSystem()->addUsedFor(
                  this->myCoordinateSystems.value(
                      feature.attribute("ref").toInt()));
            } else if (this->myTransformationParameters.contains(
                           feature.attribute("ref").toInt())) {
              myCoordinateSystem->getCoordinateSystem()->addUsedFor(
                  this->myTransformationParameters.value(
                      feature.attribute("ref").toInt()));
            }
          }
        }
      }

      // set previously needed
      QDomElement previouslyNeeded =
          coordinateSystem.firstChildElement("previouslyNeeded");
      if (!previouslyNeeded.isNull()) {
        QDomNodeList featureList = previouslyNeeded.childNodes();
        for (int j = 0; j < featureList.size(); j++) {
          QDomElement feature = featureList.at(j).toElement();
          if (feature.hasAttribute("ref")) {
            if (this->myGeometries.contains(feature.attribute("ref").toInt())) {
              myCoordinateSystem->getCoordinateSystem()->addPreviouslyNeeded(
                  this->myGeometries.value(feature.attribute("ref").toInt()));
            } else if (this->myCoordinateSystems.contains(
                           feature.attribute("ref").toInt())) {
              myCoordinateSystem->getCoordinateSystem()->addPreviouslyNeeded(
                  this->myCoordinateSystems.value(
                      feature.attribute("ref").toInt()));
            } else if (this->myTransformationParameters.contains(
                           feature.attribute("ref").toInt())) {
              myCoordinateSystem->getCoordinateSystem()->addPreviouslyNeeded(
                  this->myTransformationParameters.value(
                      feature.attribute("ref").toInt()));
            } else if (this->myStations.contains(
                           feature.attribute("ref").toInt())) {
              myCoordinateSystem->getCoordinateSystem()->addPreviouslyNeeded(
                  this->myStations.value(feature.attribute("ref").toInt()));
            }
          }
        }
      }

      // set transformation params
      QDomElement trafoParams =
          coordinateSystem.firstChildElement("transformationParameters");
      if (!trafoParams.isNull()) {
        QDomNodeList trafoParamList = trafoParams.childNodes();
        for (int j = 0; j < trafoParamList.size(); j++) {
          QDomElement trafoParam = trafoParamList.at(j).toElement();
          if (trafoParam.hasAttribute("ref") &&
              this->myTransformationParameters.contains(
                  trafoParam.attribute("ref").toInt())) {
            QPointer<FeatureWrapper> myTrafoParam =
                this->myTransformationParameters.value(
                    trafoParam.attribute("ref").toInt());
            // set the dependency for bundle system and trafoParam
            myCoordinateSystem->getCoordinateSystem()
                ->addTransformationParameter(myTrafoParam->getTrafoParam());
          }
        }
      }
    }
  }

  return result;
}

/*!
 * \brief ProjectExchanger::restoreTrafoParamDependencies
 * \param project
 * \return
 */
bool ProjectExchanger::restoreTrafoParamDependencies(
    const QDomDocument& project) {
  bool result = true;

  // iterate through the trafo param tags in the xml project
  QDomElement trafoParams =
      project.documentElement().firstChildElement("transformationParameters");
  if (!trafoParams.isNull()) {
    QDomNodeList trafoParamList = trafoParams.childNodes();
    for (int i = 0; i < trafoParamList.size(); i++) {
      // get the trafo param tag at position i and the corresponding
      // FeatureWrapper
      QDomElement trafoParam = trafoParamList.at(i).toElement();
      if (!trafoParam.hasAttribute("id") ||
          !this->myTransformationParameters.contains(
              trafoParam.attribute("id").toInt())) {
        continue;
      }
      QPointer<FeatureWrapper> myTrafoParam =
          this->myTransformationParameters.value(
              trafoParam.attribute("id").toInt());
      if (myTrafoParam.isNull() || myTrafoParam->getTrafoParam().isNull()) {
        continue;
      }

      // set start and destination system
      QDomElement from = trafoParam.firstChildElement("from");
      QDomElement to = trafoParam.firstChildElement("to");
      if (from.isNull() || to.isNull() || !from.hasAttribute("ref") ||
          !to.hasAttribute("ref") ||
          !this->myCoordinateSystems.contains(from.attribute("ref").toInt()) ||
          !this->myCoordinateSystems.contains(from.attribute("ref").toInt())) {
        continue;
      }
      myTrafoParam->getTrafoParam()->setCoordinateSystems(
          this->myCoordinateSystems.value(from.attribute("ref").toInt())
              ->getCoordinateSystem(),
          this->myCoordinateSystems.value(to.attribute("ref").toInt())
              ->getCoordinateSystem());

      // load function plugins
      QDomElement functions = trafoParam.firstChildElement("functions");
      if (!functions.isNull()) {
        QList<QPointer<Function> > myFunctions =
            this->restoreFunctionDependencies(functions);
        for (int j = 0; j < myFunctions.size(); j++) {
          myTrafoParam->getTrafoParam()->addFunction(myFunctions.at(j));
        }
      }

      // set used for
      QDomElement usedFor = trafoParam.firstChildElement("usedFor");
      if (!usedFor.isNull()) {
        QDomNodeList featureList = usedFor.childNodes();
        for (int j = 0; j < featureList.size(); j++) {
          QDomElement feature = featureList.at(j).toElement();
          if (feature.hasAttribute("ref")) {
            if (this->myGeometries.contains(feature.attribute("ref").toInt())) {
              myTrafoParam->getTrafoParam()->addUsedFor(
                  this->myGeometries.value(feature.attribute("ref").toInt()));
            } else if (this->myCoordinateSystems.contains(
                           feature.attribute("ref").toInt())) {
              myTrafoParam->getTrafoParam()->addUsedFor(
                  this->myCoordinateSystems.value(
                      feature.attribute("ref").toInt()));
            } else if (this->myTransformationParameters.contains(
                           feature.attribute("ref").toInt())) {
              myTrafoParam->getTrafoParam()->addUsedFor(
                  this->myTransformationParameters.value(
                      feature.attribute("ref").toInt()));
            }
          }
        }
      }

      // set previously needed
      QDomElement previouslyNeeded =
          trafoParam.firstChildElement("previouslyNeeded");
      if (!previouslyNeeded.isNull()) {
        QDomNodeList featureList = previouslyNeeded.childNodes();
        for (int j = 0; j < featureList.size(); j++) {
          QDomElement feature = featureList.at(j).toElement();
          if (feature.hasAttribute("ref")) {
            if (this->myGeometries.contains(feature.attribute("ref").toInt())) {
              myTrafoParam->getTrafoParam()->addPreviouslyNeeded(
                  this->myGeometries.value(feature.attribute("ref").toInt()));
            } else if (this->myCoordinateSystems.contains(
                           feature.attribute("ref").toInt())) {
              myTrafoParam->getTrafoParam()->addPreviouslyNeeded(
                  this->myCoordinateSystems.value(
                      feature.attribute("ref").toInt()));
            } else if (this->myTransformationParameters.contains(
                           feature.attribute("ref").toInt())) {
              myTrafoParam->getTrafoParam()->addPreviouslyNeeded(
                  this->myTransformationParameters.value(
                      feature.attribute("ref").toInt()));
            } else if (this->myStations.contains(
                           feature.attribute("ref").toInt())) {
              myTrafoParam->getTrafoParam()->addPreviouslyNeeded(
                  this->myStations.value(feature.attribute("ref").toInt()));
            }
          }
        }
      }

      result = true;
    }
  }

  return result;
}

/*!
 * \brief ProjectExchanger::restoreGeometryDependencies
 * \param project
 * \return
 */
bool ProjectExchanger::restoreGeometryDependencies(
    const QDomDocument& project) {
  bool result = true;

  // iterate through the geometry tags in the xml project
  QDomElement geometries =
      project.documentElement().firstChildElement("geometries");
  if (!geometries.isNull()) {
    QDomNodeList geometryList = geometries.childNodes();
    for (int i = 0; i < geometryList.size(); i++) {
      // get the geometry tag at position i and the corresponding FeatureWrapper
      QDomElement geometry = geometryList.at(i).toElement();
      if (!geometry.hasAttribute("id") ||
          !this->myGeometries.contains(geometry.attribute("id").toInt())) {
        continue;
      }
      QPointer<FeatureWrapper> myGeometry =
          this->myGeometries.value(geometry.attribute("id").toInt());
      if (myGeometry.isNull() || myGeometry->getGeometry().isNull()) {
        continue;
      }

      // set observations
      QDomElement observations = geometry.firstChildElement("observations");
      if (!observations.isNull()) {
        QDomNodeList observationList = observations.childNodes();
        for (int j = 0; j < observationList.size(); j++) {
          QDomElement observation = observationList.at(j).toElement();
          if (observation.hasAttribute("ref")) {
            QPointer<Observation> myObservation = this->myObservations.value(
                observation.attribute("ref").toInt());
            if (!myObservation.isNull()) {
              myGeometry->getGeometry()->addObservation(myObservation);
            }
          }
        }
      }

      // load function plugins
      QDomElement functions = geometry.firstChildElement("functions");
      if (!functions.isNull()) {
        QList<QPointer<Function> > myFunctions =
            this->restoreFunctionDependencies(functions);
        for (int j = 0; j < myFunctions.size(); j++) {
          myGeometry->getGeometry()->addFunction(myFunctions.at(j));
        }
      }

      // set used for
      QDomElement usedFor = geometry.firstChildElement("usedFor");
      if (!usedFor.isNull()) {
        QDomNodeList featureList = usedFor.childNodes();
        for (int j = 0; j < featureList.size(); j++) {
          QDomElement feature = featureList.at(j).toElement();
          if (feature.hasAttribute("ref")) {
            if (this->myGeometries.contains(feature.attribute("ref").toInt())) {
              myGeometry->getGeometry()->addUsedFor(
                  this->myGeometries.value(feature.attribute("ref").toInt()));
            } else if (this->myCoordinateSystems.contains(
                           feature.attribute("ref").toInt())) {
              myGeometry->getGeometry()->addUsedFor(
                  this->myCoordinateSystems.value(
                      feature.attribute("ref").toInt()));
            } else if (this->myTransformationParameters.contains(
                           feature.attribute("ref").toInt())) {
              myGeometry->getGeometry()->addUsedFor(
                  this->myTransformationParameters.value(
                      feature.attribute("ref").toInt()));
            }
          }
        }
      }

      // set previously needed
      QDomElement previouslyNeeded =
          geometry.firstChildElement("previouslyNeeded");
      if (!previouslyNeeded.isNull()) {
        QDomNodeList featureList = previouslyNeeded.childNodes();
        for (int j = 0; j < featureList.size(); j++) {
          QDomElement feature = featureList.at(j).toElement();
          if (feature.hasAttribute("ref")) {
            if (this->myGeometries.contains(feature.attribute("ref").toInt())) {
              myGeometry->getGeometry()->addPreviouslyNeeded(
                  this->myGeometries.value(feature.attribute("ref").toInt()));
            } else if (this->myCoordinateSystems.contains(
                           feature.attribute("ref").toInt())) {
              myGeometry->getGeometry()->addPreviouslyNeeded(
                  this->myCoordinateSystems.value(
                      feature.attribute("ref").toInt()));
            } else if (this->myTransformationParameters.contains(
                           feature.attribute("ref").toInt())) {
              myGeometry->getGeometry()->addPreviouslyNeeded(
                  this->myTransformationParameters.value(
                      feature.attribute("ref").toInt()));
            } else if (this->myStations.contains(
                           feature.attribute("ref").toInt())) {
              myGeometry->getGeometry()->addPreviouslyNeeded(
                  this->myStations.value(feature.attribute("ref").toInt()));
            }
          }
        }
      }

      // set what this actual realises, if anything (D17) - replaces
      // the old nominalGeometries/actual pair, which encoded a
      // bidirectional link that assumed exactly one actual per
      // nominal
      QDomElement realisesElement = geometry.firstChildElement("realises");
      if (!realisesElement.isNull() && realisesElement.hasAttribute("ref") &&
          this->myGeometries.contains(
              realisesElement.attribute("ref").toInt())) {
        QPointer<FeatureWrapper> myNominal =
            this->myGeometries.value(realisesElement.attribute("ref").toInt());
        if (!myNominal.isNull() && !myNominal->getGeometry().isNull()) {
          myGeometry->getGeometry()->setRealises(myNominal->getGeometry());
        }
      }

      // D19 applied to functions (Stage 7h): resolve the feature this
      // geometry is a bound, read-only, owned sub-geometry of, if
      // any - same two-pass ref-resolution shape as realises above.
      // Feature::fromOpenIndyXML already restored isSolved and the
      // geometry-specific fromOpenIndyXML (Line/Point/...) already
      // restored the last-computed position/direction/radius, so
      // once the owner is resolved and the geometry is solved there
      // is nothing left to (re)compute - mark it clean so opening
      // this project does not re-execute its construction chain.
      // FeatureUpdater::recalcFeatureSet's blanket isUpdated reset
      // deliberately skips owned sub-geometry for the same reason
      //(see its comment); a later real change to the owner is what
      // dirties this again (Geometry::recalc()).
      QDomElement ownerFeatureElement =
          geometry.firstChildElement("ownerFeature");
      if (!ownerFeatureElement.isNull() &&
          ownerFeatureElement.hasAttribute("ref") &&
          this->myGeometries.contains(
              ownerFeatureElement.attribute("ref").toInt())) {
        QPointer<FeatureWrapper> myOwner = this->myGeometries.value(
            ownerFeatureElement.attribute("ref").toInt());
        if (!myOwner.isNull() && !myOwner->getGeometry().isNull()) {
          if (myGeometry->getGeometry()->setOwnerFeature(
                  myOwner->getGeometry()) &&
              myGeometry->getFeature()->getIsSolved()) {
            myGeometry->getFeature()->setIsUpdated(true);
          }
        }
      }

      // set nominal system
      QDomElement nominalSystem =
          geometry.firstChildElement("nominalCoordinateSystem");
      if (!nominalSystem.isNull() && nominalSystem.hasAttribute("ref") &&
          this->myCoordinateSystems.contains(
              nominalSystem.attribute("ref").toInt())) {
        QPointer<FeatureWrapper> myNominalSystem =
            this->myCoordinateSystems.value(
                nominalSystem.attribute("ref").toInt());
        myGeometry->getGeometry()->setNominalSystem(
            myNominalSystem->getCoordinateSystem());
      }

      // set measurement configs
      QDomElement mConfigElement =
          geometry.firstChildElement("measurementConfig");
      const QVariant name = mConfigElement.attribute("name");
      if (!mConfigElement.isNull() && mConfigElement.hasAttribute("name") &&
          !name.toString().isEmpty() &&
          !isScalar(myGeometry) /* do not have a measurement config */) {
        // sensorType disambiguates configs that share a name across
        // driver types (Stage 3's whole reason for adding it to
        // MeasurementConfigKey's identity) - go through the full
        // key rather than MeasurementConfigManager::getStandardConfig(name),
        // which still only matches on name and misses every
        // driver-published standard config as a result (a separate,
        // pre-existing bug, tracked but not fixed here). SensorType
        // identity is its id() alone (SensorType::operator==), so a
        // bare id is enough to reconstruct one for lookup purposes -
        // no registry round-trip, and it works for a plugin-defined
        // type the registry was never told about, too.
        const SensorType sensorType =
            mConfigElement.hasAttribute("sensorType")
                ? SensorType(mConfigElement.attribute("sensorType"), QString())
                : SensorType();

        const MeasurementConfigKey standardKey(
            name.toString(), ConfigTypes::eStandardConfig, sensorType);
        const MeasurementConfigKey projectKey(
            name.toString(), ConfigTypes::eProjectConfig, sensorType);

        MeasurementConfig standardConfig =
            this->mConfigManager->getConfig(standardKey);
        MeasurementConfig projectConfig = this->measurementConfigs.value(
            projectKey);  // will add to MeasurementConfigManager later !!!

        if (standardConfig.isValid()) {  // if standard config with "name"
                                         // available use it
          myGeometry->getGeometry()->setMeasurementConfig(
              standardConfig.getKey());

        } else if (projectConfig.isValid()) {  // if project config with "name"
                                               // available use it
          myGeometry->getGeometry()->setMeasurementConfig(
              projectConfig.getKey());
        }
      }
    }
  }

  return result;
}

/*!
 * \brief ProjectExchanger::restoreGroupDependencies
 * Resolves Group::pendingMemberIds into real member references now that
 * every feature in the project has been loaded (Stage 7e) - the gap
 * domain-model.md's trap list records: fromOpenIndyXML only ever recorded
 * the ids, nothing resolved them, so a Group did not survive save/reload
 * at all before this. Same two-pass shape as restoreGeometryDependencies's
 * realises/ownerFeature resolution above: build a combined id->feature
 * lookup across every loaded map a member could plausibly reference (a
 * Group's accepted member types are not restricted to Geometry - see
 * Group::accepts), including myGroups itself so a group of groups
 * resolves too, then hand it to Group::restoreMembers.
 * \param project
 * \return
 */
bool ProjectExchanger::restoreGroupDependencies(const QDomDocument& project) {
  Q_UNUSED(project);

  if (this->myGroups.isEmpty()) {
    return true;
  }

  QMap<int, QPointer<FeatureWrapper> > resolvedFeatures;
  resolvedFeatures.insert(this->myGeometries);
  resolvedFeatures.insert(this->myStations);
  resolvedFeatures.insert(this->myCoordinateSystems);
  resolvedFeatures.insert(this->myTransformationParameters);
  resolvedFeatures.insert(this->myGroups);

  foreach (const QPointer<FeatureWrapper>& g, this->myGroups) {
    if (!g.isNull() && !g->getGroup().isNull()) {
      g->getGroup()->restoreMembers(resolvedFeatures);
    }
  }

  return true;
}

bool ProjectExchanger::isScalar(QPointer<FeatureWrapper> geometry) {
  return geometry->getFeatureTypeEnum() ==
             FeatureTypes::eScalarEntityAngleFeature ||
         geometry->getFeatureTypeEnum() ==
             FeatureTypes::eScalarEntityDistanceFeature ||
         geometry->getFeatureTypeEnum() ==
             FeatureTypes::eScalarEntityTemperatureFeature ||
         geometry->getFeatureTypeEnum() ==
             FeatureTypes::eScalarEntityMeasurementSeriesFeature;
}

/*!
 * \brief ProjectExchanger::restoreObservationDependencies
 * \param project
 * \return
 */
bool ProjectExchanger::restoreObservationDependencies(
    const QDomDocument& project) {
  bool result = true;

  // iterate through the observation tags in the xml project
  QDomElement observations =
      project.documentElement().firstChildElement("observations");
  if (!observations.isNull()) {
    QDomNodeList observationList = observations.childNodes();
    for (int i = 0; i < observationList.size(); i++) {
      // get the observation tag at position i and the corresponding Observation
      QDomElement observation = observationList.at(i).toElement();
      if (!observation.hasAttribute("id") ||
          !this->myObservations.contains(observation.attribute("id").toInt())) {
        continue;
      }
      QPointer<Observation> myObservation =
          this->myObservations.value(observation.attribute("id").toInt());
      if (myObservation.isNull() || myObservation->getReading().isNull()) {
        continue;
      }

      // set station and sensor
      QDomElement station = observation.firstChildElement("station");
      if (!station.isNull() && station.hasAttribute("ref") &&
          this->myStations.contains(station.attribute("ref").toInt())) {
        // get station and assign it to the observation
        QPointer<FeatureWrapper> myStation =
            this->myStations.value(station.attribute("ref").toInt());
        myObservation->setStation(myStation->getStation());
        if (myStation->getStation()->getIsSensorSet()) {
          myObservation->getReading()->setSensorConfiguration(
              myStation->getStation()->getSensorConfiguration());
        }

        // assign the observation's reading to the station
        switch (myObservation->getReading()->getTypeOfReading()) {
          case ReadingTypes::eCartesianReading:
            myStation->getStation()->cartesianReadings.append(
                myObservation->getReading());
            break;
          case ReadingTypes::ePolarReading:
            myStation->getStation()->polarReadings.append(
                myObservation->getReading());
            break;
          case ReadingTypes::eDirectionReading:
            myStation->getStation()->directionReadings.append(
                myObservation->getReading());
            break;
          case ReadingTypes::eDistanceReading:
            myStation->getStation()->distanceReadings.append(
                myObservation->getReading());
            break;
          case ReadingTypes::eLevelReading:
            myStation->getStation()->levelReadings.append(
                myObservation->getReading());
            break;
          case ReadingTypes::eTemperatureReading:
            myStation->getStation()->temperatureRadings.append(
                myObservation->getReading());
            break;
        }
      }

      // set measuredTargetGeometry
      QDomElement measuredTargetGeom =
          observation.firstChildElement("measuredTargetGeometry");
      if (!measuredTargetGeom.isNull() &&
          measuredTargetGeom.hasAttribute("ref") &&
          this->myGeometries.contains(
              measuredTargetGeom.attribute("ref").toInt())) {
        // get geometry and assign it
        QPointer<FeatureWrapper> myMeasuredTargetGeom =
            this->myGeometries.value(
                measuredTargetGeom.attribute("ref").toInt());
        myObservation->setMeasuredTargetGeometry(
            myMeasuredTargetGeom->getGeometry());
      }
    }
  }

  return result;
}

/*!
 * \brief ProjectExchanger::restoreFunctionDependencies
 * Helper method which gets a functions tag as input and then returns a list of
 * function objects ordered by their execution index
 * \param functions
 * \return
 */
QList<QPointer<Function> > ProjectExchanger::restoreFunctionDependencies(
    const QDomElement& functions) {
  QList<QPointer<Function> > result;

  if (!functions.isNull()) {
    // get a list of all function tags
    QDomNodeList functionList = functions.childNodes();

    // map to save all found functions with their executionIndex as key
    QMap<int, QPointer<Function> > functionOrder;

    for (int i = 0; i < functionList.size(); i++) {
      // get a function tag
      QDomElement function = functionList.at(i).toElement();
      if (!function.hasAttribute("name") || !function.hasAttribute("plugin") ||
          !function.hasAttribute("executionIndex")) {
        continue;
      }

      // load function plugin
      std::optional<sys::Plugin> plugin =
          PluginRepository::getByName(function.attribute("plugin"));
      QPointer<Function> myFunction(nullptr);
      if (plugin.has_value()) {
        myFunction = PluginLoader::loadFunctionPlugin(
            plugin->filePath(), function.attribute("name"));
        if (myFunction.isNull()) {
          continue;
        }
      } else {
        continue;
      }

      // load function from XML
      myFunction->fromOpenIndyXML(function);

      // add input elements
      QDomElement inputElements = function.firstChildElement("inputElements");
      if (!inputElements.isNull()) {
        QDomNodeList elementList = inputElements.childNodes();
        for (int j = 0; j < elementList.size(); j++) {
          QDomElement inputElement = elementList.at(j).toElement();

          // get shouldBeUsed
          bool shouldBeUsed = true;
          if (inputElement.hasAttribute("shouldBeUsed")) {
            shouldBeUsed = inputElement.attribute("shouldBeUsed").toInt();
          }

          // set ignored destination parameters

          // create the list of ignored parameters
          QList<GeometryParameters> restoreIgnoredParameter;
          // look for the ignored destination parameter section in the xml file
          QDomElement ignoredDestinationParams =
              inputElement.firstChildElement("ignoredDestinationParams");
          // fill the list with ignored parameters
          if (!ignoredDestinationParams.isNull()) {
            QDomNodeList ignoredParamList =
                ignoredDestinationParams.childNodes();
            for (int a = 0; a < ignoredParamList.size(); a++) {
              QDomElement ignore = ignoredParamList.at(a).toElement();

              if (ignore.hasAttribute("parameter")) {
                restoreIgnoredParameter.append(
                    getGeometryParameterEnum(ignore.attribute("parameter")));
              }
            }
          }

          // create and add input elements
          if (inputElement.hasAttribute("index") &&
              inputElement.hasAttribute("type") &&
              inputElement.hasAttribute("ref")) {
            if (this->myStations.contains(
                    inputElement.attribute("ref").toInt())) {
              QPointer<Station> station =
                  this->myStations.value(inputElement.attribute("ref").toInt())
                      ->getStation();
              InputElement element;
              element.station = station;
              element.id = station->getId();
              element.typeOfElement = ElementTypes::eStationElement;
              element.shouldBeUsed = shouldBeUsed;
              element.ignoredDestinationParams = restoreIgnoredParameter;
              myFunction->addInputElement(
                  element, inputElement.attribute("index").toInt());
            } else if (this->myCoordinateSystems.contains(
                           inputElement.attribute("ref").toInt())) {
              QPointer<CoordinateSystem> coordinateSystem =
                  this->myCoordinateSystems
                      .value(inputElement.attribute("ref").toInt())
                      ->getCoordinateSystem();
              InputElement element;
              element.coordSystem = coordinateSystem;
              element.id = coordinateSystem->getId();
              element.typeOfElement = ElementTypes::eCoordinateSystemElement;
              element.shouldBeUsed = shouldBeUsed;
              element.ignoredDestinationParams = restoreIgnoredParameter;
              myFunction->addInputElement(
                  element, inputElement.attribute("index").toInt());
            } else if (this->myTransformationParameters.contains(
                           inputElement.attribute("ref").toInt())) {
              QPointer<TrafoParam> trafoParam =
                  this->myTransformationParameters
                      .value(inputElement.attribute("ref").toInt())
                      ->getTrafoParam();
              InputElement element;
              element.trafoParam = trafoParam;
              element.id = trafoParam->getId();
              element.typeOfElement = ElementTypes::eTrafoParamElement;
              element.shouldBeUsed = shouldBeUsed;
              element.ignoredDestinationParams = restoreIgnoredParameter;
              myFunction->addInputElement(
                  element, inputElement.attribute("index").toInt());
            } else if (this->myGeometries.contains(
                           inputElement.attribute("ref").toInt())) {
              QPointer<FeatureWrapper> geometry = this->myGeometries.value(
                  inputElement.attribute("ref").toInt());
              InputElement element;
              element.geometry = geometry->getGeometry();
              element.id = geometry->getGeometry()->getId();
              element.shouldBeUsed = shouldBeUsed;
              element.ignoredDestinationParams = restoreIgnoredParameter;
              switch (geometry->getFeatureTypeEnum()) {
                case FeatureTypes::eCircleFeature:
                  element.circle = geometry->getCircle();
                  element.typeOfElement = ElementTypes::eCircleElement;
                  break;
                case FeatureTypes::eConeFeature:
                  element.cone = geometry->getCone();
                  element.typeOfElement = ElementTypes::eConeElement;
                  break;
                case FeatureTypes::eCylinderFeature:
                  element.cylinder = geometry->getCylinder();
                  element.typeOfElement = ElementTypes::eCylinderElement;
                  break;
                case FeatureTypes::eEllipseFeature:
                  element.ellipse = geometry->getEllipse();
                  element.typeOfElement = ElementTypes::eEllipseElement;
                  break;
                case FeatureTypes::eEllipsoidFeature:
                  element.ellipsoid = geometry->getEllipsoid();
                  element.typeOfElement = ElementTypes::eEllipsoidElement;
                  break;
                case FeatureTypes::eHyperboloidFeature:
                  element.hyperboloid = geometry->getHyperboloid();
                  element.typeOfElement = ElementTypes::eHyperboloidElement;
                  break;
                case FeatureTypes::eLineFeature:
                  element.line = geometry->getLine();
                  element.typeOfElement = ElementTypes::eLineElement;
                  break;
                case FeatureTypes::eNurbsFeature:
                  element.nurbs = geometry->getNurbs();
                  element.typeOfElement = ElementTypes::eNurbsElement;
                  break;
                case FeatureTypes::eParaboloidFeature:
                  element.paraboloid = geometry->getParaboloid();
                  element.typeOfElement = ElementTypes::eParaboloidElement;
                  break;
                case FeatureTypes::ePlaneFeature:
                  element.plane = geometry->getPlane();
                  element.typeOfElement = ElementTypes::ePlaneElement;
                  break;
                case FeatureTypes::ePointFeature:
                  element.point = geometry->getPoint();
                  element.typeOfElement = ElementTypes::ePointElement;
                  break;
                case FeatureTypes::ePointCloudFeature:
                  element.pointCloud = geometry->getPointCloud();
                  element.typeOfElement = ElementTypes::ePointCloudElement;
                  break;
                case FeatureTypes::eScalarEntityAngleFeature:
                  element.scalarEntityAngle = geometry->getScalarEntityAngle();
                  element.typeOfElement =
                      ElementTypes::eScalarEntityAngleElement;
                  break;
                case FeatureTypes::eScalarEntityDistanceFeature:
                  element.scalarEntityDistance =
                      geometry->getScalarEntityDistance();
                  element.typeOfElement =
                      ElementTypes::eScalarEntityDistanceElement;
                  break;
                case FeatureTypes::eScalarEntityMeasurementSeriesFeature:
                  element.scalarEntityMeasurementSeries =
                      geometry->getScalarEntityMeasurementSeries();
                  element.typeOfElement =
                      ElementTypes::eScalarEntityMeasurementSeriesElement;
                  break;
                case FeatureTypes::eScalarEntityTemperatureFeature:
                  element.scalarEntityTemperature =
                      geometry->getScalarEntityTemperature();
                  element.typeOfElement =
                      ElementTypes::eScalarEntityTemperatureElement;
                  break;
                case FeatureTypes::eSlottedHoleFeature:
                  element.slottedHole = geometry->getSlottedHole();
                  element.typeOfElement = ElementTypes::eSlottedHoleElement;
                  break;
                case FeatureTypes::eSphereFeature:
                  element.sphere = geometry->getSphere();
                  element.typeOfElement = ElementTypes::eSphereElement;
                  break;
                case FeatureTypes::eTorusFeature:
                  element.torus = geometry->getTorus();
                  element.typeOfElement = ElementTypes::eTorusElement;
                  break;
              }
              myFunction->addInputElement(
                  element, inputElement.attribute("index").toInt());
            } else if (this->myObservations.contains(
                           inputElement.attribute("ref").toInt())) {
              QPointer<Observation> observation = this->myObservations.value(
                  inputElement.attribute("ref").toInt());
              InputElement element;
              element.observation = observation;
              element.id = observation->getId();
              element.typeOfElement = ElementTypes::eObservationElement;
              element.shouldBeUsed = shouldBeUsed;
              element.ignoredDestinationParams = restoreIgnoredParameter;
              myFunction->addInputElement(
                  element, inputElement.attribute("index").toInt());
            } else if (this->myReadings.contains(
                           inputElement.attribute("ref").toInt())) {
              QPointer<Reading> reading =
                  this->myReadings.value(inputElement.attribute("ref").toInt());
              InputElement element;
              element.shouldBeUsed = shouldBeUsed;
              element.ignoredDestinationParams = restoreIgnoredParameter;
              switch (reading->getTypeOfReading()) {
                case ReadingTypes::eDistanceReading:
                  element.distanceReading = reading;
                  element.typeOfElement = ElementTypes::eReadingDistanceElement;
                  break;
                case ReadingTypes::eCartesianReading:
                  element.cartesianReading = reading;
                  element.typeOfElement =
                      ElementTypes::eReadingCartesianElement;
                  break;
                case ReadingTypes::ePolarReading:
                  element.polarReading = reading;
                  element.typeOfElement = ElementTypes::eReadingPolarElement;
                  break;
                case ReadingTypes::eDirectionReading:
                  element.directionReading = reading;
                  element.typeOfElement =
                      ElementTypes::eReadingDirectionElement;
                  break;
                case ReadingTypes::eTemperatureReading:
                  element.temperatureReading = reading;
                  element.typeOfElement =
                      ElementTypes::eReadingTemperatureElement;
                  break;
                case ReadingTypes::eLevelReading:
                  element.levelReading = reading;
                  element.typeOfElement = ElementTypes::eReadingLevelElement;
                  break;
              }
              element.id = reading->getId();
              myFunction->addInputElement(
                  element, inputElement.attribute("index").toInt());
            }
          }
        }
      }

      // add the function to the functionOrder map
      functionOrder.insert(function.attribute("executionIndex").toInt(),
                           myFunction);
    }

    // iterate through all functions in functionOrder map
    for (int i = 0; i < functionOrder.size(); i++) {
      if (!functionOrder.contains(i)) {
        break;
      }
      result.append(functionOrder.value(i));
    }
  }

  return result;
}

/*!
 * \brief ProjectExchanger::restoreBundleDependencies
 * Helper method which gets a bundle adjustment tag as input and then returns a
 * pointer to the bundle adjustment
 * \param bundle
 * \return
 */
QPointer<BundleAdjustment> ProjectExchanger::restoreBundleDependencies(
    QDomElement& bundle) {
  QPointer<BundleAdjustment> result;

  if (!bundle.isNull()) {
    if (!bundle.hasAttribute("name") || !bundle.hasAttribute("plugin")) {
      return result;
    }

    std::optional<sys::Plugin> plugin =
        PluginRepository::getByName(bundle.attribute("plugin"));
    QPointer<BundleAdjustment> myBundle(nullptr);
    if (plugin.has_value()) {
      myBundle = PluginLoader::loadBundleAdjustmentPlugin(
          plugin->filePath(), bundle.attribute("name"));
      if (myBundle.isNull()) {
        return result;
      }
    } else {
      return result;
    }

    // load bundle from xml
    myBundle->fromOpenIndyXML(bundle);

    // add bundleCoordinateSystem
    QDomElement bundleCoordSys =
        bundle.firstChildElement("bundleCoordinateSystem");
    if (!bundleCoordSys.isNull() && bundleCoordSys.hasAttribute("ref")) {
      QPointer<FeatureWrapper> mySystem = this->myCoordinateSystems.value(
          bundleCoordSys.attribute("ref").toInt());
      if (!mySystem.isNull() && !mySystem->getCoordinateSystem().isNull()) {
        myBundle->setBundleSystem(mySystem->getCoordinateSystem());
        result = myBundle;
      } else {
        return result;
      }
    } else {
      return result;
    }
  }
  return result;
}

/*!
 * \brief ProjectExchanger::clearHelperMaps
 * Clear the helper maps for loading an OpenIndy project. If deleteOnClear is
 * true then the list objects are not only removed but also deleted.
 * \param deleteOnClear
 */
void ProjectExchanger::clearHelperMaps(bool deleteOnClear) {
  // delete the helper maps objects
  if (deleteOnClear) {
    // delete observations
    QList<QPointer<Observation> > myObservations =
        this->myObservations.values();
    foreach (QPointer<Observation> obs, myObservations) {
      delete obs;
    }

    // delete stations
    QList<QPointer<FeatureWrapper> > myStations = this->myStations.values();
    foreach (QPointer<FeatureWrapper> s, myStations) {
      delete s->getStation();
      delete s;
    }

    // delete coordinate systems
    QList<QPointer<FeatureWrapper> > myCoordinateSystems =
        this->myCoordinateSystems.values();
    foreach (QPointer<FeatureWrapper> c, myCoordinateSystems) {
      delete c->getCoordinateSystem();
      delete c;
    }

    // delete trafo params
    QList<QPointer<FeatureWrapper> > myTrafoParams =
        this->myTransformationParameters.values();
    foreach (QPointer<FeatureWrapper> t, myTrafoParams) {
      delete t->getTrafoParam();
      delete t;
    }

    // delete geometries
    QList<QPointer<FeatureWrapper> > myGeometries = this->myGeometries.values();
    foreach (QPointer<FeatureWrapper> g, myGeometries) {
      delete g->getFeature();
      delete g;
    }

    // delete groups
    QList<QPointer<FeatureWrapper> > myGroups = this->myGroups.values();
    foreach (QPointer<FeatureWrapper> g, myGroups) {
      delete g->getFeature();
      delete g;
    }
  }

  // clear the helper maps
  this->myObservations.clear();
  this->myStations.clear();
  this->myCoordinateSystems.clear();
  this->myReadings.clear();
  this->myTransformationParameters.clear();
  this->myGeometries.clear();
  this->myGroups.clear();
  this->measurementConfigs.clear();
  this->mySConfigs.clear();
  this->stationPoints.clear();
}

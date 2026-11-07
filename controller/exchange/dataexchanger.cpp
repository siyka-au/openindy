#include "dataexchanger.h"

#include <vector>

#include "containers.h"
#include "qtconvert.h"

/*!
 * \brief DataExchanger::DataExchanger
 * \param parent
 */
DataExchanger::DataExchanger(QObject* parent) : QObject(parent) {}

/*!
 * \brief DataExchanger::~DataExchanger
 */
DataExchanger::~DataExchanger() {
  // stop thread if it is still running
  if (this->exchangeThread.isRunning()) {
    this->exchangeThread.quit();
    this->exchangeThread.wait();
  }

  // an exchange that is still around must not report to a dead exchanger
  if (!this->exchange.isNull()) {
    this->exchange->setMessageSink(nullptr);
    this->exchange->setObserver(nullptr);
  }

  this->discardExchangeRunner();
}

/*!
 * \brief DataExchanger::discardExchangeRunner
 * Deletes the object whose thread affinity carries the exchange task; the
 * exchange thread must not be running.
 */
void DataExchanger::discardExchangeRunner() {
  delete this->exchangeRunner;
  this->exchangeRunner = nullptr;
}

/*!
 * \brief DataExchanger::runOnExchangeThread
 * Runs `work` once on the exchange thread (the exchange plugin is no longer a
 * QObject that could be moved there and have a slot invoked).
 */
void DataExchanger::runOnExchangeThread(std::function<void()> work) {
  this->discardExchangeRunner();
  this->exchangeRunner = new QObject();
  QObject::connect(&this->exchangeThread, &QThread::started,
                   this->exchangeRunner, std::move(work));
  this->exchangeRunner->moveToThread(&this->exchangeThread);
  this->exchangeThread.start();
}

/*!
 * \brief DataExchanger::getCurrentJob
 * \return
 */
const oi::Guarded<OiJob>& DataExchanger::getCurrentJob() const {
  return this->currentJob;
}

/*!
 * \brief DataExchanger::setCurrentJob
 * \param job
 */
void DataExchanger::setCurrentJob(const oi::Guarded<OiJob>& job) {
  this->currentJob = job;
}

/*!
 * \brief DataExchanger::getMeasurementConfigManager
 * \return
 */
const QPointer<MeasurementConfigManager>&
DataExchanger::getMeasurementConfigManager() {
  return this->mConfigManager;
}

/*!
 * \brief DataExchanger::setMesaurementConfigManager
 * \param manager
 */
void DataExchanger::setMesaurementConfigManager(
    const QPointer<MeasurementConfigManager>& manager) {
  this->mConfigManager = manager;
}

/*!
 * \brief DataExchanger::setExchangeParams
 * \param exchangeParams
 */
void DataExchanger::setExchangeParams(ExchangeParams exchangeParams) {
  this->exchangeParams = exchangeParams;
}

/*!
 * \brief DataExchanger::importData
 * \param params
 * \return
 */
bool DataExchanger::importData(const ExchangeParams& params) {
  // quit the thread if it is still running
  if (this->exchangeThread.isRunning()) {
    this->exchangeThread.quit();
    this->exchangeThread.wait();
  }

  // check current job
  if (this->currentJob.isNull()) {
    emit this->sendMessage("No job specified", MessageTypes::eErrorMessage,
                           MessageDestinations::eMessageBoxMessage);
    return false;
  }

  // check nominal system
  std::vector<oi::Guarded<FeatureWrapper>> features =
      this->currentJob->getFeaturesByName(params.nominalSystem);
  if (features.size() != 1 || features.at(0).isNull() ||
      features.at(0)->getCoordinateSystem().isNull() ||
      features.at(0)->getCoordinateSystem()->getIsStationSystem()) {
    emit this->sendMessage("No valid nominal system",
                           MessageTypes::eErrorMessage,
                           MessageDestinations::eMessageBoxMessage);
    return false;
  }

  // check if given plugin name is empty
  if (params.pluginName.compare("") == 0 ||
      params.exchangeName.compare("") == 0) {
    emit this->sendMessage(
        QString("No exchange available with the name %1 in the plugin %2")
            .arg(params.exchangeName)
            .arg(params.pluginName),
        MessageTypes::eErrorMessage, MessageDestinations::eMessageBoxMessage);
    return false;
  }

  // get the plugin and check if it is valid
  std::optional<sys::Plugin> plugin =
      PluginRepository::getByName(params.pluginName);
  if (!plugin.has_value()) {
    emit this->sendMessage("No valid plugin specified",
                           MessageTypes::eErrorMessage,
                           MessageDestinations::eMessageBoxMessage);
    return false;
  }

  // load exchange method with plugin loader
  oi::Guarded<ExchangeInterface> exchange(nullptr);
  if (params.isDefinedFormat) {
    oi::Guarded<ExchangeDefinedFormat> definedFormat =
        PluginLoader::loadExchangeDefinedFormatPlugin(plugin->filePath(),
                                                      params.exchangeName);
    if (!definedFormat.isNull()) {
      definedFormat->setUsedElements(params.usedElements);
    }
    exchange = definedFormat;
  } else {
    oi::Guarded<ExchangeSimpleAscii> simpleAscii =
        PluginLoader::loadExchangeSimpleAsciiPlugin(plugin->filePath(),
                                                    params.exchangeName);
    if (!simpleAscii.isNull()) {
      switch (params.typeOfGeometry) {
        case GeometryTypes::ePointGeometry: {
          simpleAscii->setGeometryType(params.typeOfGeometry);
          simpleAscii->setSkipFirstLine(params.skipFirstLine);
          simpleAscii->setDelimiter(params.delimiter);

          // set user defined columns here hard coded (later selectable in table
          // view)
          // TODO user defined columns
          std::vector<ExchangeSimpleAscii::ColumnType> columns;
          columns.push_back(ExchangeSimpleAscii::eColumnFeatureName);
          columns.push_back(ExchangeSimpleAscii::eColumnX);
          columns.push_back(ExchangeSimpleAscii::eColumnY);
          columns.push_back(ExchangeSimpleAscii::eColumnZ);
          if (params.readCommonColumn) {
            columns.push_back(ExchangeSimpleAscii::eColumnCommonState);
          }
          simpleAscii->setUserDefinedColumns(columns);

          // simpleAscii->setUserDefinedColumns(params.userDefinedColumns);
          break;
        }
        case GeometryTypes::ePlaneGeometry:
        case GeometryTypes::ePlaneLevelGeometry: {
          simpleAscii->setGeometryType(params.typeOfGeometry);
          simpleAscii->setSkipFirstLine(params.skipFirstLine);
          simpleAscii->setDelimiter(params.delimiter);

          std::vector<ExchangeSimpleAscii::ColumnType> columns;
          columns.push_back(ExchangeSimpleAscii::eColumnFeatureName);
          columns.push_back(ExchangeSimpleAscii::eColumnX);
          columns.push_back(ExchangeSimpleAscii::eColumnY);
          columns.push_back(ExchangeSimpleAscii::eColumnZ);
          columns.push_back(ExchangeSimpleAscii::eColumnPrimaryI);
          columns.push_back(ExchangeSimpleAscii::eColumnPrimaryJ);
          columns.push_back(ExchangeSimpleAscii::eColumnPrimaryK);
          simpleAscii->setUserDefinedColumns(columns);

          /* not working, because of visability
           * simpleAscii->setUserDefinedColumns(simpleAscii->getDefaultColumnOrder(ePlaneGeometry));
           */

          break;
        }
      }
    }
    exchange = simpleAscii;
  }

  // check if the loaded exchange method is ok
  if (exchange.isNull()) {
    emit this->sendMessage(
        QString("No exchange available with the name %1 in the plugin %2")
            .arg(params.exchangeName)
            .arg(params.pluginName),
        MessageTypes::eErrorMessage, MessageDestinations::eMessageBoxMessage);
    return false;
  }
  this->exchange = exchange;

  // set exchange parameters
  this->exchangeStream = std::make_unique<std::fstream>(
      toPath(params.exchangeFilePath), std::ios::in | std::ios::binary);
  this->exchange->setStream(this->exchangeStream.get());
  this->exchange->setFeatures(params.exportFeatures);
  this->exchange->setExportObservations(params.exportObservations);
  this->exchange->setGroupName(params.groupName);
  this->exchange->setNominalSystem(features.at(0)->getCoordinateSystem());
  std::vector<DimensionType> dimensions = oi::keys(params.units);
  foreach (const DimensionType& dimension, dimensions) {
    this->exchange->setUnit(dimension, oi::value(params.units, dimension));
  }

  // connect exchange plugin
  this->exporting = false;
  this->exchange->setMessageSink(this);
  this->exchange->setObserver(this);
  // run the import on the exchange thread
  this->runOnExchangeThread([exchange = this->exchange]() {
    if (!exchange.isNull()) exchange->importOiData();
  });

  return true;
}

/*!
 * \brief DataExchanger::exportData
 * \param params
 * \return
 */
bool DataExchanger::exportData(const ExchangeParams& params) {
  // quit the thread if it is still running
  if (this->exchangeThread.isRunning()) {
    this->exchangeThread.quit();
    this->exchangeThread.wait();
  }

  // check current job
  if (this->currentJob.isNull()) {
    emit this->sendMessage("No job specified", MessageTypes::eErrorMessage,
                           MessageDestinations::eMessageBoxMessage);
    return false;
  }

  // check if given plugin name is empty
  if (params.pluginName.compare("") == 0 ||
      params.exchangeName.compare("") == 0) {
    emit this->sendMessage(
        QString("No exchange available with the name %1 in the plugin %2")
            .arg(params.exchangeName)
            .arg(params.pluginName),
        MessageTypes::eErrorMessage, MessageDestinations::eMessageBoxMessage);
    return false;
  }

  // get the plugin and check if it is valid
  std::optional<sys::Plugin> plugin =
      PluginRepository::getByName(params.pluginName);
  if (!plugin.has_value()) {
    emit this->sendMessage("No valid plugin specified",
                           MessageTypes::eErrorMessage,
                           MessageDestinations::eMessageBoxMessage);
    return false;
  }

  // load exchange method with plugin loader
  oi::Guarded<ExchangeInterface> exchange(nullptr);
  if (params.isDefinedFormat) {
    oi::Guarded<ExchangeDefinedFormat> definedFormat =
        PluginLoader::loadExchangeDefinedFormatPlugin(
            qApp->applicationDirPath() + "/" + plugin->filePath(),
            params.exchangeName);
    if (!definedFormat.isNull()) {
      definedFormat->setUsedElements(params.usedElements);
    }
    exchange = definedFormat;
  } else {
    oi::Guarded<ExchangeSimpleAscii> simpleAscii =
        PluginLoader::loadExchangeSimpleAsciiPlugin(
            qApp->applicationDirPath() + "/" + plugin->filePath(),
            params.exchangeName);
    if (!simpleAscii.isNull()) {
      simpleAscii->setGeometryType(params.typeOfGeometry);
      simpleAscii->setSkipFirstLine(params.skipFirstLine);
      simpleAscii->setDelimiter(params.delimiter);
      for (const auto& [dimension, unit] : params.units) {
        simpleAscii->setUnit(dimension, unit);
      }
      simpleAscii->setDistanceDigits(params.distanceDigits);
      simpleAscii->setAngleDigits(params.angleDigits);
      simpleAscii->setTemperatureDigits(params.angleDigits);

      // set user defined columns here hard coded (later selectable in table
      // view)
      // TODO user defined columns
      std::vector<ExchangeSimpleAscii::ColumnType> columns;
      columns.push_back(ExchangeSimpleAscii::eColumnFeatureName);
      columns.push_back(ExchangeSimpleAscii::eColumnX);
      columns.push_back(ExchangeSimpleAscii::eColumnY);
      columns.push_back(ExchangeSimpleAscii::eColumnZ);
      columns.push_back(ExchangeSimpleAscii::eColumnPrimaryI);
      columns.push_back(ExchangeSimpleAscii::eColumnPrimaryJ);
      columns.push_back(ExchangeSimpleAscii::eColumnPrimaryK);
      columns.push_back(ExchangeSimpleAscii::eColumnRadiusA);

      simpleAscii->setUserDefinedColumns(columns);
      simpleAscii->setCurrentJob(this->currentJob);

      // simpleAscii->setUserDefinedColumns(params.userDefinedColumns);
    }
    exchange = simpleAscii;
  }

  // check if the loaded exchange method is ok
  if (exchange.isNull()) {
    emit this->sendMessage(
        QString("No exchange available with the name %1 in the plugin %2")
            .arg(params.exchangeName)
            .arg(params.pluginName),
        MessageTypes::eErrorMessage, MessageDestinations::eMessageBoxMessage);
    return false;
  }
  this->exchange = exchange;

  // set exchange parameters
  this->exchangeStream = std::make_unique<std::fstream>(
      toPath(params.exchangeFilePath),
      std::ios::out | std::ios::trunc | std::ios::binary);
  this->exchange->setStream(this->exchangeStream.get());
  this->exchange->setFeatures(params.exportFeatures);

  // connect exchange plugin
  this->exporting = true;
  this->exchange->setMessageSink(this);
  this->exchange->setObserver(this);
  // run the export on the exchange thread
  this->runOnExchangeThread([exchange = this->exchange]() {
    if (!exchange.isNull()) exchange->exportOiData();
  });

  return true;
}

/*!
 * \brief DataExchanger::importObservations
 * \param filename
 * \return
 */
bool DataExchanger::importObservations(const QString& filename) {
  // quit the thread if it is still running
  if (this->exchangeThread.isRunning()) {
    this->exchangeThread.quit();
    this->exchangeThread.wait();
  }

  // check current job and pass it to observation importer
  if (this->currentJob.isNull()) {
    emit this->sendMessage("No job specified", MessageTypes::eErrorMessage,
                           MessageDestinations::eMessageBoxMessage);
    return false;
  }

  // no stale runner may fire when the thread starts again
  this->discardExchangeRunner();

  // create observation importer and pass the current job
  this->observationImporter = new ObservationImporter();
  this->observationImporter->setCurrentJob(this->currentJob);
  this->observationImporter->setFileName(filename);

  // connect observation importer
  QObject::connect(&this->exchangeThread, &QThread::started,
                   this->observationImporter.data(),
                   &ObservationImporter::importObservations,
                   Qt::AutoConnection);
  QObject::connect(
      this->observationImporter.data(), &ObservationImporter::importFinished,
      this, &DataExchanger::importObservationsFinished, Qt::AutoConnection);
  QObject::connect(this->observationImporter.data(),
                   &ObservationImporter::updateProgress, this,
                   &DataExchanger::updateObservationImportProgress,
                   Qt::AutoConnection);
  QObject::connect(this->observationImporter.data(),
                   &ObservationImporter::sendMessage, this,
                   &DataExchanger::sendMessage, Qt::AutoConnection);

  // move exchange plugin to thread and start data exchange
  this->observationImporter->moveToThread(&this->exchangeThread);
  this->exchangeThread.start();

  return true;
}

void DataExchanger::importMeasurements(
    std::vector<oi::Guarded<FeatureWrapper>> features) {
  const auto curDateTime = std::chrono::system_clock::now();
  foreach (oi::Guarded<FeatureWrapper> importedFeature, features) {
    std::vector<oi::Guarded<FeatureWrapper>> jobFeatures =
        this->currentJob->getFeaturesByName(
            importedFeature->getFeature()->getFeatureName());
    if (jobFeatures.empty()) {
      qDebug() << "features not found: "
               << importedFeature->getFeature()->getFeatureName();
      continue;
    }

    foreach (oi::Guarded<FeatureWrapper> jobFeature, jobFeatures) {
      if (!jobFeature->getGeometry()->getOwnerDescription().has_value()) {
        qDebug() << "isNominal: "
                 << jobFeature->getGeometry()->getFeatureName();
        continue;
      }

      std::vector<oi::Guarded<Reading>> importedReadings;
      oi::Guarded<Reading> reading = nullptr;

      switch (jobFeature->getFeatureTypeEnum()) {
        case FeatureTypes::ePointFeature: {
          // Points / Cartesian
          OiVec p = importedFeature->getPoint()->getPosition().getVector();

          ReadingCartesian rCartesian;
          rCartesian.xyz.setAt(0, p.getAt(0));
          rCartesian.xyz.setAt(1, p.getAt(1));
          rCartesian.xyz.setAt(2, p.getAt(2));
          rCartesian.isValid = true;
          reading = new Reading(rCartesian);

          break;
        }
        case FeatureTypes::ePlaneFeature: {
          if (importedFeature->getFeature()
                  ->getTransientAttribute("OI_FEATURE_PLANE_LEVEL")
                  .toBool()) {
            // Level
            OiVec direction =
                importedFeature->getPlane()->getDirection().getVector();

            ReadingLevel rLevel;
            rLevel.i = direction.getAt(0);
            rLevel.j = direction.getAt(1);
            rLevel.k = direction.getAt(2);
            rLevel.isValid = true;
            reading = new Reading(rLevel);

          } else {
            // Points / Cartesian
            OiVec p = importedFeature->getPoint()->getPosition().getVector();

            ReadingCartesian rCartesian;
            rCartesian.xyz.setAt(0, p.getAt(0));
            rCartesian.xyz.setAt(1, p.getAt(1));
            rCartesian.xyz.setAt(2, p.getAt(2));
            rCartesian.isValid = true;
            reading = new Reading(rCartesian);
          }
          break;
        }
      }

      reading->setSensorFace(SensorFaces::eUndefinedSide);
      reading->setMeasuredAt(curDateTime);
      reading->setImported(true);

      importedReadings.push_back(reading);

      this->currentJob->addMeasurementResults(
          jobFeature->getGeometry()->getId(), importedReadings);
      emit this->sendMessage(
          QString("import reading to feature: \"%1\"")
              .arg(jobFeature->getFeature()->getFeatureName()),
          MessageTypes::eInformationMessage,
          MessageDestinations::eConsoleMessage);
    }
  }
}

void DataExchanger::createActuals(
    std::vector<oi::Guarded<FeatureWrapper>> features) {
  foreach (oi::Guarded<FeatureWrapper> fw, features) {
    switch (fw->getFeatureTypeEnum()) {
      case FeatureTypes::ePlaneFeature:
      case FeatureTypes::ePointFeature: {
        FeatureAttributes fAttr;
        fAttr.count = 1;
        fAttr.typeOfFeature = fw->getFeatureTypeEnum();
        fAttr.name = fw->getFeature()->getFeatureName();
        fAttr.group = fw->getFeature()->getGroupName();

        fAttr.isCommon = !QString::compare(
            fw->getFeature()
                ->getTransientAttribute("OI_FEATURE_COMMONSTATE")
                .toString(),
            "true", Qt::CaseInsensitive);

        QString mcElementTypeName;
        FeatureTypes funFeatureType;
        if (fw->getFeature()
                ->getTransientAttribute("OI_FEATURE_PLANE_LEVEL")
                .toBool()) {  // property is still needed because there is no
                              // separate FeatureType for Level
          mcElementTypeName = getElementTypeName(ElementTypes::eLevelElement);
          funFeatureType = FeatureTypes::eLevelFeature;
        } else {
          mcElementTypeName = fw->getFeatureTypeString();
          funFeatureType = fAttr.typeOfFeature;
        }

        QString mConfigName = Settings::get(
            QString("defaultMeasurementConfig.%1").arg(mcElementTypeName));
        MeasurementConfig mConfig = mConfigManager->findConfig(mConfigName);

        sys::Function defaultFunction =
            PluginRepository::getDefaultFunction(funFeatureType)
                .value_or(sys::Function());
        std::pair<QString, QString> functionPlugin;
        functionPlugin.first = defaultFunction.name();
        functionPlugin.second = defaultFunction.plugin().filePath();
        fAttr.functionPlugin = functionPlugin;

        std::vector<oi::Guarded<FeatureWrapper>> addedFeatures =
            this->currentJob->addFeatures(fAttr);

        this->addFunctionsAndMConfigs(addedFeatures, mConfig,
                                      fAttr.functionPlugin.second,
                                      fAttr.functionPlugin.first);

        // this workflow's whole point is "add actuals to nominals at
        // import" (D17): link the new actual to a same-named, same-
        // type nominal already in the job, if there is one
        if (addedFeatures.size() == 1 && !addedFeatures.front().isNull() &&
            !addedFeatures.front()->getGeometry().isNull()) {
          foreach (const oi::Guarded<FeatureWrapper>& candidate,
                   this->currentJob->getFeaturesByName(fAttr.name)) {
            if (!candidate.isNull() && !candidate->getGeometry().isNull() &&
                candidate->getFeatureTypeEnum() == fAttr.typeOfFeature &&
                !candidate->getGeometry()->getOwnerDescription().has_value()) {
              addedFeatures.front()->getGeometry()->setRealises(
                  candidate->getGeometry());
              break;
            }
          }
        }

        break;
      }  // case

    }  // switch
  }
}

void DataExchanger::runOnOwnThread(std::function<void()> work) {
  if (QThread::currentThread() == this->thread()) {
    work();
  } else {
    QMetaObject::invokeMethod(this, std::move(work), Qt::QueuedConnection);
  }
}

void DataExchanger::onMessage(const QString& msg, const MessageTypes& type,
                              const MessageDestinations& dest) {
  this->runOnOwnThread(
      [this, msg, type, dest] { emit this->sendMessage(msg, type, dest); });
}

void DataExchanger::onProgress(int percent, const QString& message) {
  // only an import has ever shown progress
  this->runOnOwnThread([this, percent, message] {
    if (!this->exporting) {
      emit this->updateNominalImportProgress(percent, message);
    }
  });
}

void DataExchanger::onImportFinished(bool success) {
  this->runOnOwnThread([this, success] { this->importFeatures(success); });
}

void DataExchanger::onExportFinished(bool success) {
  this->runOnOwnThread([this, success] { this->exportFeatures(success); });
}

/*!
 * \brief DataExchanger::importFeatures
 * \param success
 */
void DataExchanger::importFeatures(const bool& success) {
  if (!success) {
    emit this->nominalImportFinished(false);
    return;
  }

  // add the imported features to current OpenIndy job
  std::vector<oi::Guarded<FeatureWrapper>> features =
      this->exchange->getFeatures();

  bool importSuccess = false;
  if (this->exchangeParams.importMeasurements) {
    // import only reading to existing actuals
    importMeasurements(features);
    importSuccess = true;
  } else {
    // add features
    importSuccess =
        this->currentJob->addFeatures(features, this->exchangeParams.overwrite);

    // add actuals to nominals at import
    if (this->exchangeParams.createActual) {
      createActuals(features);
    }

    // get the specified coordinate system by its defined name from exchange
    // parameters and set this one true so destination system from import
    // nominals is active system and all nominal values will be shown directly
    std::vector<oi::Guarded<CoordinateSystem>> coordSystemList =
        this->currentJob->getCoordinateSystemsList();
    foreach (oi::Guarded<CoordinateSystem> cs, coordSystemList) {
      if (cs->getFeatureName().compare(this->exchangeParams.nominalSystem) ==
          0) {
        this->currentJob->getActiveCoordinateSystem()
            ->setActiveCoordinateSystemState(false);
        cs->setActiveCoordinateSystemState(true);
      }
    }
  }

  // delete exchange instance because it is not necessary anymore
  delete this->exchange;

  // stop thread if it is still running
  if (this->exchangeThread.isRunning()) {
    this->exchangeThread.quit();
    this->exchangeThread.wait();
  }

  emit this->nominalImportFinished(importSuccess);
}

/*!
 * \brief DataExchanger::exportFeatures
 * \param success
 */
void DataExchanger::exportFeatures(const bool& success) {
  // delete exchange instance because it is not necessary anymore
  delete this->exchange;

  // stop thread if it is still running
  if (this->exchangeThread.isRunning()) {
    this->exchangeThread.quit();
    this->exchangeThread.wait();
  }

  if (success) {
    emit this->sendMessage("Features successfully exported",
                           MessageTypes::eInformationMessage,
                           MessageDestinations::eMessageBoxMessage);
  } else {
    emit this->sendMessage("Features not exported successfully",
                           MessageTypes::eErrorMessage,
                           MessageDestinations::eMessageBoxMessage);
  }
}

/*!
 * \brief DataExchanger::importObservationsFinished
 * \param success
 */
void DataExchanger::importObservationsFinished(const bool& success) {
  if (!success) {
    emit this->observationImportFinished(false);
    delete this->observationImporter;
    return;
  }

  // delete observation importer instance because it is not necessary anymore
  delete this->observationImporter;

  // stop thread if it is still running
  if (this->exchangeThread.isRunning()) {
    this->exchangeThread.quit();
    this->exchangeThread.wait();
  }

  emit this->observationImportFinished(success);
}

/*!
 * \brief DataExchanger::addFunctionsAndMConfigs
 * \param actuals
 * \param mConfig
 * \param path
 * \param fName
 */
void DataExchanger::addFunctionsAndMConfigs(
    const std::vector<oi::Guarded<FeatureWrapper>>& actuals,
    const MeasurementConfig& mConfig, const QString& path,
    const QString& fName) {
  // check job
  if (this->currentJob.isNull()) {
    return;
  }

  foreach (const oi::Guarded<FeatureWrapper>& feature, actuals) {
    // check feature
    if (feature.isNull() || feature->getFeature().isNull()) {
      continue;
    }

    // load function plugin
    oi::Guarded<Function> function(nullptr);
    if (!path.isEmpty() && !fName.isEmpty()) {
      function = PluginLoader::loadFunctionPlugin(path, fName);
    }

    // assign function and measurement config to feature
    if (!function.isNull()) {
      feature->getFeature()->addFunction(function);
    }
    if (mConfig.isValid() && !feature->getGeometry().isNull()) {
      feature->getGeometry()->setMeasurementConfig(mConfig.getKey());
    }
  }
}

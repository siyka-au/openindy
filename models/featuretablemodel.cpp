#include "featuretablemodel.h"

#include <map>
#include <vector>

#include "containers.h"
#include "jobconnect.h"
#include "qtjobadapter.h"

/*!
 * \brief FeatureTableModel::FeatureTableModel
 * \param job
 * \param parent
 */
FeatureTableModel::FeatureTableModel(const oi::Guarded<OiJob>& job,
                                     QObject* parent)
    : QAbstractTableModel(parent) {
  this->setCurrentJob(job);
}

/*!
 * \brief FeatureTableModel::FeatureTableModel
 * \param parent
 */
FeatureTableModel::FeatureTableModel(QObject* parent)
    : QAbstractTableModel(parent) {}

/*!
 * \brief FeatureTableModel::rowCount
 * \param parent
 * \return
 */
int FeatureTableModel::rowCount(const QModelIndex& parent) const {
  if (this->currentJob.isNull()) {
    return 0;
  }
  return this->currentJob->getFeatureCount();
}

/*!
 * \brief FeatureTableModel::columnCount
 * \param parent
 * \return
 */
int FeatureTableModel::columnCount(const QModelIndex& parent) const {
  return getFeatureDisplayAttributes().size();
}

/*!
 * \brief FeatureTableModel::data
 * \param index
 * \param role
 * \return
 */
QVariant FeatureTableModel::data(const QModelIndex& index, int role) const {
  // check current job and model index
  if (this->currentJob.isNull() || !index.isValid()) {
    return QVariant();
  }

  // get row and column indices
  int rowIndex = index.row();
  int columnIndex = index.column();

  // get the feature to display at index.row()
  if (this->currentJob->getFeatureCount() <= rowIndex) {
    return QVariant();
  }
  oi::Guarded<FeatureWrapper> feature =
      this->currentJob->getFeaturesList().at(rowIndex);

  // check the feature
  if (feature.isNull() || feature->getFeature().isNull()) {
    return QVariant();
  }

  if (role == Qt::DisplayRole) {
    // return the string to display for the feature at index.column()
    return this->getDisplayValue(feature, columnIndex);

  } else if (role == Qt::BackgroundRole) {
    // return the color in which to display the feature at index.column()
    return this->getBackgroundValue(feature, columnIndex);

  } else if (role == Qt::ForegroundRole) {
    // return the text color to use against the (possibly custom) background at
    // index.column()
    return this->getForegroundValue(feature, columnIndex);

  } else if (role == Qt::CheckStateRole && !feature->getGeometry().isNull() &&
             feature->getGeometry()->getOwnerDescription().has_value()) {
    // get the display attribute
    int attr = getFeatureDisplayAttributes().at(columnIndex);

    // get common state and solved
    switch ((FeatureDisplayAttributes)attr) {
      case eFeatureDisplayIsCommon:
        if (feature->getGeometry()->getIsCommon()) {
          return Qt::Checked;
        }
        return Qt::Unchecked;
      case eFeatureDisplayIsSolved:
        if (feature->getGeometry()->getIsSolved()) {
          return Qt::Checked;
        }
        return Qt::Unchecked;
    }

  } else if (role == Qt::CheckStateRole && !feature->getTrafoParam().isNull()) {
    int attr = getFeatureDisplayAttributes().at(columnIndex);

    // get isSolved from TrafoParam
    switch ((TrafoParamDisplayAttributes)attr) {
      case eTrafoParamDisplayIsSolved:
        if (feature->getTrafoParam()->getIsSolved()) {
          return Qt::Checked;
        }
        return Qt::Unchecked;
      case eTrafoParamDisplayIsUsed:
        if (feature->getTrafoParam()->getIsUsed()) {
          return Qt::Checked;
        }
        return Qt::Unchecked;
      default:
        break;
    }

  } else if (role == Qt::TextAlignmentRole &&
             (!feature->getGeometry().isNull() ||
              !feature->getCoordinateSystem().isNull() ||
              !feature->getStation().isNull())) {
    switch ((FeatureDisplayAttributes)getFeatureDisplayAttributes().at(
        columnIndex)) {
      case eFeatureDisplayStDev:
      case eFeatureDisplayX:
      case eFeatureDisplayY:
      case eFeatureDisplayZ:
      case eFeatureDisplayPrimaryI:
      case eFeatureDisplayPrimaryJ:
      case eFeatureDisplayPrimaryK:
      case eFeatureDisplayRadiusA:
      case eFeatureDisplayRadiusB:
      case eFeatureDisplaySecondaryI:
      case eFeatureDisplaySecondaryJ:
      case eFeatureDisplaySecondaryK:
      case eFeatureDisplayAperture:
      case eFeatureDisplayA:
      case eFeatureDisplayB:
      case eFeatureDisplayC:
      case eFeatureDisplayAngle:
      case eFeatureDisplayDistance:
      case eFeatureDisplayTemperature:
      case eFeatureDisplayLength:
      case eFeatureDisplayExpansionOriginX:
      case eFeatureDisplayExpansionOriginY:
      case eFeatureDisplayExpansionOriginZ:
      case eFeatureDisplayFormError:
        return QVariant(Qt::AlignRight | Qt::AlignVCenter);
    }

  } else if (role == Qt::TextAlignmentRole &&
             !feature->getTrafoParam().isNull()) {
    switch ((TrafoParamDisplayAttributes)getFeatureDisplayAttributes().at(
        columnIndex)) {
      case eTrafoParamDisplayTranslationX:
      case eTrafoParamDisplayTranslationY:
      case eTrafoParamDisplayTranslationZ:
      case eTrafoParamDisplayRotationX:
      case eTrafoParamDisplayRotationY:
      case eTrafoParamDisplayRotationZ:
      case eTrafoParamDisplayScaleX:
      case eTrafoParamDisplayScaleY:
      case eTrafoParamDisplayScaleZ:
      case eTrafoParamDisplayStDev:
        return QVariant(Qt::AlignRight | Qt::AlignVCenter);
    }
  }

  return QVariant();
}

/*!
 * \brief FeatureTableModel::headerData
 * \param section
 * \param orientation
 * \param role
 * \return
 */
QVariant FeatureTableModel::headerData(int section, Qt::Orientation orientation,
                                       int role) const {
  if ((Qt::DisplayRole == role) && (Qt::Horizontal == orientation) &&
      (0 <= section) && (section < columnCount())) {
    // get header
    QString header = getFeatureDisplayAttributeName(
        getFeatureDisplayAttributes().at(section));

    // add unit information to header
    if (getIsFeatureDisplayAttribute(
            getFeatureDisplayAttributes().at(section))) {
      switch (
          (FeatureDisplayAttributes)getFeatureDisplayAttributes().at(section)) {
        case eFeatureDisplayStDev:
          header.append(QString(" %1").arg(
              getUnitTypeName(this->parameterDisplayConfig.getDisplayUnit(
                  DimensionType::eMetric))));
          break;
        case eFeatureDisplayX:
          header.append(QString(" %1").arg(
              getUnitTypeName(this->parameterDisplayConfig.getDisplayUnit(
                  DimensionType::eMetric))));
          break;
        case eFeatureDisplayY:
          header.append(QString(" %1").arg(
              getUnitTypeName(this->parameterDisplayConfig.getDisplayUnit(
                  DimensionType::eMetric))));
          break;
        case eFeatureDisplayZ:
          header.append(QString(" %1").arg(
              getUnitTypeName(this->parameterDisplayConfig.getDisplayUnit(
                  DimensionType::eMetric))));
          break;
        case eFeatureDisplayRadiusA:
          header.append(QString(" %1").arg(
              getUnitTypeName(this->parameterDisplayConfig.getDisplayUnit(
                  DimensionType::eMetric))));
          break;
        case eFeatureDisplayRadiusB:
          header.append(QString(" %1").arg(
              getUnitTypeName(this->parameterDisplayConfig.getDisplayUnit(
                  DimensionType::eMetric))));
          break;
        case eFeatureDisplayAperture:
          header.append(QString(" %1").arg(
              getUnitTypeName(this->parameterDisplayConfig.getDisplayUnit(
                  DimensionType::eAngular))));
          break;
        case eFeatureDisplayA:
          header.append(QString(" %1").arg(
              getUnitTypeName(this->parameterDisplayConfig.getDisplayUnit(
                  DimensionType::eMetric))));
          break;
        case eFeatureDisplayB:
          header.append(QString(" %1").arg(
              getUnitTypeName(this->parameterDisplayConfig.getDisplayUnit(
                  DimensionType::eMetric))));
          break;
        case eFeatureDisplayC:
          header.append(QString(" %1").arg(
              getUnitTypeName(this->parameterDisplayConfig.getDisplayUnit(
                  DimensionType::eMetric))));
          break;
        case eFeatureDisplayAngle:
          header.append(QString(" %1").arg(
              getUnitTypeName(this->parameterDisplayConfig.getDisplayUnit(
                  DimensionType::eAngular))));
          break;
        case eFeatureDisplayDistance:
          header.append(QString(" %1").arg(
              getUnitTypeName(this->parameterDisplayConfig.getDisplayUnit(
                  DimensionType::eMetric))));
          break;
        case eFeatureDisplayTemperature:
          header.append(QString(" %1").arg(
              getUnitTypeName(this->parameterDisplayConfig.getDisplayUnit(
                  DimensionType::eTemperature))));
          break;
        case eFeatureDisplayLength:
          header.append(QString(" %1").arg(
              getUnitTypeName(this->parameterDisplayConfig.getDisplayUnit(
                  DimensionType::eMetric))));
          break;
        case eFeatureDisplayExpansionOriginX:
          header.append(QString(" %1").arg(
              getUnitTypeName(this->parameterDisplayConfig.getDisplayUnit(
                  DimensionType::eMetric))));
          break;
        case eFeatureDisplayExpansionOriginY:
          header.append(QString(" %1").arg(
              getUnitTypeName(this->parameterDisplayConfig.getDisplayUnit(
                  DimensionType::eMetric))));
          break;
        case eFeatureDisplayExpansionOriginZ:
          header.append(QString(" %1").arg(
              getUnitTypeName(this->parameterDisplayConfig.getDisplayUnit(
                  DimensionType::eMetric))));
          break;
        case eFeatureDisplayFormError:
          header.append(QString(" %1").arg(
              getUnitTypeName(this->parameterDisplayConfig.getDisplayUnit(
                  DimensionType::eMetric))));
          break;
      }

    } else if (getIsTrafoParamDisplayAttribute(
                   getFeatureDisplayAttributes().at(section))) {
      switch ((TrafoParamDisplayAttributes)getFeatureDisplayAttributes().at(
          section)) {
        case eTrafoParamDisplayStDev:
          header.append(QString(" %1").arg(
              getUnitTypeName(this->parameterDisplayConfig.getDisplayUnit(
                  DimensionType::eMetric))));
          break;
        case eTrafoParamDisplayTranslationX:
          header.append(QString(" %1").arg(
              getUnitTypeName(this->parameterDisplayConfig.getDisplayUnit(
                  DimensionType::eMetric))));
          break;
        case eTrafoParamDisplayTranslationY:
          header.append(QString(" %1").arg(
              getUnitTypeName(this->parameterDisplayConfig.getDisplayUnit(
                  DimensionType::eMetric))));
          break;
        case eTrafoParamDisplayTranslationZ:
          header.append(QString(" %1").arg(
              getUnitTypeName(this->parameterDisplayConfig.getDisplayUnit(
                  DimensionType::eMetric))));
          break;
        case eTrafoParamDisplayRotationX:
          header.append(QString(" %1").arg(
              getUnitTypeName(this->parameterDisplayConfig.getDisplayUnit(
                  DimensionType::eAngular))));
          break;
        case eTrafoParamDisplayRotationY:
          header.append(QString(" %1").arg(
              getUnitTypeName(this->parameterDisplayConfig.getDisplayUnit(
                  DimensionType::eAngular))));
          break;
        case eTrafoParamDisplayRotationZ:
          header.append(QString(" %1").arg(
              getUnitTypeName(this->parameterDisplayConfig.getDisplayUnit(
                  DimensionType::eAngular))));
          break;
      }
    }
    return header;
  }
  return QVariant();
}

/*!
 * \brief FeatureTableModel::flags
 * Add edit-functionality to some cells of tablemodel
 * \param index
 * \return
 */
Qt::ItemFlags FeatureTableModel::flags(const QModelIndex& index) const {
  // get parent flags
  Qt::ItemFlags myFlags = QAbstractTableModel::flags(index);

  // check index
  if (!index.isValid()) {
    return myFlags;
  }

  // get display attribute
  int rowIndex = index.row();
  int columnIndex = index.column();
  int attr = getFeatureDisplayAttributes().at(columnIndex);

  // get the feature to display at index.row()
  if (this->currentJob->getFeatureCount() <= rowIndex) {
    return myFlags;
  }
  oi::Guarded<FeatureWrapper> feature =
      this->currentJob->getFeaturesList().at(rowIndex);

  // check the feature
  if (feature.isNull() || feature->getFeature().isNull()) {
    return myFlags;
  }

  if (getIsFeatureDisplayAttribute(attr)) {
    // get and check display attribute
    FeatureDisplayAttributes fAttr = (FeatureDisplayAttributes)attr;
    if (fAttr == eFeatureDisplayName || fAttr == eFeatureDisplayComment ||
        fAttr == eFeatureDisplayGroup) {
      return (myFlags | Qt::ItemIsEditable);
    } else if (fAttr == eFeatureDisplayIsCommon &&
               !feature->getGeometry().isNull() &&
               feature->getGeometry()->getOwnerDescription().has_value()) {
      return (myFlags | Qt::ItemIsEditable | Qt::ItemIsUserCheckable);
    } else if (fAttr == eFeatureDisplayIsSolved) {
      return (myFlags | Qt::ItemIsUserCheckable);
    } else if (!feature->getGeometry().isNull() &&
               !feature->getGeometry()->getOwnerDescription().has_value()) {
      if (fAttr == eFeatureDisplayX || fAttr == eFeatureDisplayY ||
          fAttr == eFeatureDisplayZ || fAttr == eFeatureDisplayPrimaryI ||
          eFeatureDisplayPrimaryJ || fAttr == eFeatureDisplayPrimaryK) {
        return (myFlags | Qt::ItemIsEditable);
      }
    }

  } else if (getIsTrafoParamDisplayAttribute(attr)) {
    // get and check display attribute
    TrafoParamDisplayAttributes fAttr = (TrafoParamDisplayAttributes)attr;

    if (fAttr == eTrafoParamDisplayIsSolved) {
      return (myFlags | Qt::ItemIsUserCheckable);
    } else if (
        fAttr ==
        eTrafoParamDisplayIsUsed) {  // else if(fAttr ==
                                     // eTrafoParamDisplayIsUsed || fAttr ==
                                     // eTrafoParamDisplayIsDatumTransformation){
      return (myFlags | Qt::ItemIsEditable | Qt::ItemIsUserCheckable);
    }
  }
  return myFlags;
}

/*!
 * \brief FeatureTableModel::setData
 * Update the edited features
 * \param index
 * \param value
 * \param role
 * \return
 */
bool FeatureTableModel::setData(const QModelIndex& index, const QVariant& value,
                                int role) {
  return this->setData(index, value, role,
                       EditMode::eFunctionCopyScalarInputParams);
}

bool FeatureTableModel::setData(const QModelIndex& index, const QVariant& value,
                                int role, EditModes editMode) {
  // check current job and model index
  if (this->currentJob.isNull() || !index.isValid()) {
    return false;
  }

  // get the feature to at index.row()
  if (this->currentJob->getFeatureCount() <= index.row()) {
    return false;
  }
  oi::Guarded<FeatureWrapper> feature =
      this->currentJob->getFeaturesList().at(index.row());

  // check the feature
  if (feature.isNull() || feature->getFeature().isNull()) {
    return false;
  }

  // get and check column index
  int column = index.column();
  if (column < 0 || getFeatureDisplayAttributes().size() <= column) {
    return false;
  }

  // get display attribute
  int attr = getFeatureDisplayAttributes().at(column);

  // if a non-trafo param feature has been edited
  if (getIsFeatureDisplayAttribute(attr) && role == Qt::EditRole) {
    switch ((FeatureDisplayAttributes)attr) {
      case eFeatureDisplayName: {
        // D17: nominal and actual are ordinary features now, not a
        // name-matched pair, so there is no special-case to block here
        // any more - only commit the new feature name if it is valid
        if (!this->currentJob->validateFeatureName(
                value.toString(), feature->getFeatureTypeEnum())) {
          return false;
        }

        // commit the new feature name
        feature->getFeature()->setFeatureName(value.toString());

        return true;
      }
      case eFeatureDisplayComment: {
        feature->getFeature()->setComment(value.toString());
        return true;
      }
      case eFeatureDisplayGroup: {
        feature->getFeature()->setGroupName(value.toString());
        return true;
      }
      case eFeatureDisplayMeasurementConfig: {
        // check if the feature is a geometry
        if (feature->getGeometry().isNull()) {
          return false;
        }

        // get the feature to copy measurement config from
        oi::Guarded<FeatureWrapper> copyFeature =
            this->currentJob->getFeatureById(value.toInt());
        // 0. precondition: function(s) available ?
        if (!copyFeature.isNull() && !copyFeature->getFeature().isNull() &&
            !copyFeature->getFeature()->getFunctions().size() == 0) {
          if (copyFeature->getFeature()->getId() ==
              feature->getFeature()->getId()) {
            // dont overwrite the measurement config of the feature you copy
            // from.
            return false;
          }

          feature->getFeature()->setActiveFeatureState(true);

          feature->getGeometry()->setMeasurementConfig(
              copyFeature->getGeometry()->getMeasurementConfig());

          return true;
        }
        break;
      }
      case eFeatureDisplayFunctions: {
        // check if the feature is a geometry
        if (feature->getGeometry().isNull() &&
            feature->getTrafoParam().isNull()) {
          return false;
        }

        // only actual features have a function
        if (!feature->getGeometry().isNull() &&
            !feature->getGeometry()->getOwnerDescription().has_value()) {
          return false;
        }

        // get the feature to copy functions from
        oi::Guarded<FeatureWrapper> copyFeature =
            this->currentJob->getFeatureById(value.toInt());

        // 0. precondition: function(s) available ?
        if (!copyFeature.isNull() && !copyFeature->getFeature().isNull() &&
            !copyFeature->getFeature()->getFunctions().size() == 0) {
          if (copyFeature->getFeature()->getId() ==
              feature->getFeature()->getId()) {
            // dont overwrite the functions of the feature you copy from. If you
            // do, you will reset the parameters to default values.
            return false;
          }

          if (copyFeature->getFeatureTypeEnum() !=
              feature->getFeatureTypeEnum()) {
            QMessageBox msgBox;
            msgBox.setText(QString("Cannot copy functions from feature type %1 "
                                   "to feature type %2.")
                               .arg(copyFeature->getFeatureTypeString())
                               .arg(feature->getFeatureTypeString()));
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.setDefaultButton(QMessageBox::Ok);
            int ret = msgBox.exec();

            return false;
          }

          // 1. set current feature as "active feature" to make currentJob-calls
          // work on the right feature :)
          feature->getFeature()->setActiveFeatureState(true);

          // 2. remove all functions if copy functions available
          const int functionCount =
              feature->getFeature()->getFunctions().size();
          for (int i = 0; i < functionCount; i++) {
            this->currentJob->removeFunction(
                0);  // remove index 0 from active feature
          }

          // 3.1 loop over all copy / source function
          bool result = false;
          foreach (oi::Guarded<Function> copyFunction,
                   copyFeature->getFeature()->getFunctions()) {
            oi::Guarded<Function> function =
                loadFunctionByName(copyFunction->getMetaData().name);

            if (!function.isNull()) {
              result = true;

              // 3.2 add new function
              this->currentJob->addFunction(function);

              // 3.3 assign function parameters from copied function to the new
              // functions

              // 3.3.1 copy scalar input params
              if (editMode.testFlag(EditMode::eFunctionCopyScalarInputParams)) {
                function->setScalarInputParams(
                    copyFunction->getScalarInputParams());
              }  // <- copy scalar input params

              oi::Guarded<Geometry> geometry = feature->getGeometry();
              // 3.3.2 copy used elements
              if (editMode.testFlag(EditMode::eFunctionCopyUsedElements)) {
                std::map<int, std::vector<InputElement> > inputElements =
                    copyFunction->getInputElements();
                std::map<int, std::vector<InputElement> >::const_iterator
                    iterator = inputElements.cbegin();
                if (!geometry.isNull() &&
                    !inputElements.empty()) {  // remove existing observation
                                               // references
                  for (oi::Guarded<Observation> obs :
                       geometry->getObservations()) {
                    geometry->removeObservation(obs);
                  }
                }
                while (iterator != inputElements.cend()) {
                  std::vector<InputElement> elements = iterator->second;
                  for (auto element : elements) {
                    function->addInputElement(element, iterator->first);
                    if (!geometry.isNull() &&
                        !element.observation
                             .isNull()) {  // insert observation reference from
                                           // function to feature
                      geometry->addObservation(element.observation);
                    }
                  }
                  ++iterator;
                }
              }  // <- copy used elements
            }
          }
          return result;
        }
        break;
      }
      case eFeatureDisplayX:
        if (!feature->getGeometry().isNull() &&
            !feature->getGeometry()->getOwnerDescription().has_value()) {
          std::map<GeometryParameters, double> parameters;
          parameters.insert_or_assign(
              GeometryParameters::eUnknownX,
              convertToDefault(value.toDouble(),
                               this->parameterDisplayConfig.getDisplayUnit(
                                   DimensionType::eMetric)));
          feature->getGeometry()->setUnknownParameters(parameters);
          emit recalcActiveFeature();
          return true;
        }
        break;
      case eFeatureDisplayY:
        if (!feature->getGeometry().isNull() &&
            !feature->getGeometry()->getOwnerDescription().has_value()) {
          std::map<GeometryParameters, double> parameters;
          parameters.insert_or_assign(
              GeometryParameters::eUnknownY,
              convertToDefault(value.toDouble(),
                               this->parameterDisplayConfig.getDisplayUnit(
                                   DimensionType::eMetric)));
          feature->getGeometry()->setUnknownParameters(parameters);
          emit recalcActiveFeature();
          return true;
        }
        break;
      case eFeatureDisplayZ:
        if (!feature->getGeometry().isNull() &&
            !feature->getGeometry()->getOwnerDescription().has_value()) {
          std::map<GeometryParameters, double> parameters;
          parameters.insert_or_assign(
              GeometryParameters::eUnknownZ,
              convertToDefault(value.toDouble(),
                               this->parameterDisplayConfig.getDisplayUnit(
                                   DimensionType::eMetric)));
          feature->getGeometry()->setUnknownParameters(parameters);
          emit recalcActiveFeature();
          return true;
        }
        break;
      case eFeatureDisplayPrimaryI:
        if (!feature->getGeometry().isNull() &&
            !feature->getGeometry()->getOwnerDescription().has_value()) {
          std::map<GeometryParameters, double> parameters;
          parameters.insert_or_assign(
              GeometryParameters::eUnknownPrimaryI,
              convertToDefault(value.toDouble(),
                               this->parameterDisplayConfig.getDisplayUnit(
                                   DimensionType::eAngular)));
          feature->getGeometry()->setUnknownParameters(parameters);
          emit recalcActiveFeature();
          return true;
        }
        break;
      case eFeatureDisplayPrimaryJ:
        if (!feature->getGeometry().isNull() &&
            !feature->getGeometry()->getOwnerDescription().has_value()) {
          std::map<GeometryParameters, double> parameters;
          parameters.insert_or_assign(
              GeometryParameters::eUnknownPrimaryJ,
              convertToDefault(value.toDouble(),
                               this->parameterDisplayConfig.getDisplayUnit(
                                   DimensionType::eAngular)));
          feature->getGeometry()->setUnknownParameters(parameters);
          emit recalcActiveFeature();
          return true;
        }
        break;
      case eFeatureDisplayPrimaryK:
        if (!feature->getGeometry().isNull() &&
            !feature->getGeometry()->getOwnerDescription().has_value()) {
          std::map<GeometryParameters, double> parameters;
          parameters.insert_or_assign(
              GeometryParameters::eUnknownPrimaryK,
              convertToDefault(value.toDouble(),
                               this->parameterDisplayConfig.getDisplayUnit(
                                   DimensionType::eAngular)));
          feature->getGeometry()->setUnknownParameters(parameters);
          emit recalcActiveFeature();
          return true;
        }
        break;
    }

  } else if (getIsTrafoParamDisplayAttribute(attr) && role == Qt::EditRole) {
    switch ((TrafoParamDisplayAttributes)attr) {
      case eTrafoParamDisplayName: {
        // only commit the new feature name if it is valid
        if (!this->currentJob->validateFeatureName(
                value.toString(), feature->getFeatureTypeEnum())) {
          return false;
        }

        // commit the new feature name
        feature->getFeature()->setFeatureName(value.toString());
        return true;
      }
      case eTrafoParamDisplayComment: {
        feature->getFeature()->setComment(value.toString());
        return true;
      }
      case eTrafoParamDisplayGroup: {
        feature->getFeature()->setGroupName(value.toString());
        return true;
      }
    }

  } else if (!feature->getGeometry().isNull() && role == Qt::CheckStateRole) {
    switch ((FeatureDisplayAttributes)attr) {
      case eFeatureDisplayIsCommon: {
        bool isCommon = value.toBool();
        feature->getGeometry()->setCommonState(isCommon);
        emit recalcActiveFeature();
        break;
      }
    }

  } else if (!feature->getTrafoParam().isNull() && role == Qt::CheckStateRole) {
    switch ((TrafoParamDisplayAttributes)attr) {
      case eTrafoParamDisplayIsUsed: {
        bool isUsed = value.toBool();
        oi::Guarded<TrafoParam> curTrafoParam = feature->getTrafoParam();

        if (isUsed) {  // first uncheck other
          for (oi::Guarded<FeatureWrapper> f :
               this->currentJob->getFeaturesByType(
                   FeatureTypes::eTrafoParamFeature)) {
            oi::Guarded<TrafoParam> trafoParam = f->getTrafoParam();
            if (trafoParam.isNull() ||
                !trafoParam->getIsUsed()  // is already false
                || curTrafoParam->getId() == trafoParam->getId()  // it's me
                || curTrafoParam->getStartSystem() !=
                       trafoParam->getStartSystem()  // no match
                || curTrafoParam->getDestinationSystem() !=
                       trafoParam->getDestinationSystem()  // no match
            ) {
              continue;
            }

            bool oldState = trafoParam->blockSignals(true);
            trafoParam->setIsUsed(false);
            trafoParam->blockSignals(oldState);
          }
        }

        // than set "isUsed" and emit signal
        curTrafoParam->setIsUsed(isUsed);

        break;
      } /*case eTrafoParamDisplayIsDatumTransformation:{
           bool isDatum = value.toBool();
           feature->getTrafoParam()->setIsDatumTrafo(isDatum);
           break;
       }*/
      default:
        break;
    }
  }

  return false;
}

oi::Guarded<Function> FeatureTableModel::loadFunctionByName(QString name) {
  //*******************************************************
  // work around : get all functions and compare the names. then get the
  // function plugin path to create the function. the plugin path is not stored
  // correctly in the metadata of the functions (It is a bug!!), thats why this
  // workaraound is needed
  //*******************************************************
  oi::Guarded<Function> function(nullptr);

  foreach (const sys::Function& f, PluginRepository::getAllFunctions()) {
    if (f.name() == name) {
      function =
          PluginLoader::loadFunctionPlugin(f.plugin().filePath(), f.name());
      break;
    }
  }
  return function;
}

/*!
 * \brief FeatureTableModel::getFeatureIdAtIndex
 * Returns the id of the feature at index or -1
 * \param index
 * \return
 */
int FeatureTableModel::getFeatureIdAtIndex(const QModelIndex& index) {
  // check index
  if (!index.isValid()) {
    return -1;
  }

  // check current job
  if (this->currentJob.isNull()) {
    return -1;
  }

  // check if row index is valid
  if (index.row() >= this->currentJob->getFeatureCount()) {
    return -1;
  }

  // check feature at index
  if (this->currentJob->getFeaturesList().at(index.row()).isNull() ||
      this->currentJob->getFeaturesList()
          .at(index.row())
          ->getFeature()
          .isNull()) {
    return -1;
  }

  return this->currentJob->getFeaturesList()
      .at(index.row())
      ->getFeature()
      ->getId();
}

/*!
 * \brief FeatureTableModel::getActiveFeature
 * \return
 */
oi::Guarded<FeatureWrapper> FeatureTableModel::getActiveFeature() const {
  // check current job
  if (this->currentJob.isNull()) {
    return oi::Guarded<FeatureWrapper>(nullptr);
  }

  return this->currentJob->getActiveFeature();
}

/*!
 * \brief FeatureTableModel::setActiveFeature
 * \param index
 */
void FeatureTableModel::setActiveFeature(const QModelIndex& index) {
  // check index
  if (!index.isValid()) {
    return;
  }

  // check current job
  if (this->currentJob.isNull()) {
    return;
  }

  // check if row index is valid
  if (index.row() >= this->currentJob->getFeatureCount()) {
    return;
  }

  // check feature at index
  if (this->currentJob->getFeaturesList().at(index.row()).isNull() ||
      this->currentJob->getFeaturesList()
          .at(index.row())
          ->getFeature()
          .isNull()) {
    return;
  }

  // set active feature
  this->currentJob->getFeaturesList()
      .at(index.row())
      ->getFeature()
      ->setActiveFeatureState(true);
}

/*!
 * \brief FeatureTableModel::setActiveFeature
 * \param index
 */
void FeatureTableModel::setActiveFeature(const int& id) {
  // check current job
  if (this->currentJob.isNull()) {
    return;
  }

  // get and check feature with the given id
  oi::Guarded<FeatureWrapper> feature = this->currentJob->getFeatureById(id);
  if (feature.isNull() || feature->getFeature().isNull()) {
    return;
  }

  // set active feature
  feature->getFeature()->setActiveFeatureState(true);
}

/*!
 * \brief FeatureTableModel::getActiveStation
 * \return
 */
oi::Guarded<Station> FeatureTableModel::getActiveStation() const {
  // check current job
  if (this->currentJob.isNull()) {
    return oi::Guarded<Station>(nullptr);
  }

  return this->currentJob->getActiveStation();
}

/*!
 * \brief FeatureTableModel::setActiveStation
 * \param index
 */
void FeatureTableModel::setActiveStation(const QModelIndex& index) {
  // check index
  if (!index.isValid()) {
    return;
  }

  // check current job
  if (this->currentJob.isNull()) {
    return;
  }

  // check if row index is valid
  if (index.row() >= this->currentJob->getFeatureCount()) {
    return;
  }

  // check feature at index
  oi::Guarded<FeatureWrapper> feature =
      this->currentJob->getFeaturesList().at(index.row());
  if (feature.isNull() || feature->getStation().isNull()) {
    emit this->sendMessage("No station selected",
                           MessageTypes::eWarningMessage);
    return;
  }

  // set active station
  feature->getStation()->setActiveStationState(true);
}

/*!
 * \brief FeatureTableModel::setActiveStation
 * \param id
 */
void FeatureTableModel::setActiveStation(const int& id) {
  // check current job
  if (this->currentJob.isNull()) {
    return;
  }

  // get and check feature with the given id
  oi::Guarded<FeatureWrapper> feature = this->currentJob->getFeatureById(id);
  if (feature.isNull() || feature->getStation().isNull()) {
    emit this->sendMessage("No station selected",
                           MessageTypes::eWarningMessage);
    return;
  }

  // set active station
  feature->getStation()->setActiveStationState(true);
}

/*!
 * \brief FeatureTableModel::getActiveCoordinateSystem
 * \return
 */
oi::Guarded<CoordinateSystem> FeatureTableModel::getActiveCoordinateSystem()
    const {
  // check current job
  if (this->currentJob.isNull()) {
    return oi::Guarded<CoordinateSystem>(nullptr);
  }

  return this->currentJob->getActiveCoordinateSystem();
}

/*!
 * \brief FeatureTableModel::setActiveCoordinateSystem
 * \param name
 */
void FeatureTableModel::setActiveCoordinateSystem(const QString& name) {
  // check current job
  if (this->currentJob.isNull()) {
    return;
  }

  // get and check feature with the given name
  std::vector<oi::Guarded<FeatureWrapper> > features =
      this->currentJob->getFeaturesByName(name);
  if (features.size() != 1) {
    return;
  }
  oi::Guarded<FeatureWrapper> feature = features.at(0);

  // get and check coordinate system
  oi::Guarded<CoordinateSystem> system(nullptr);
  if (!feature->getStation().isNull() && !feature->getStation()
                                              ->getCoordinateSystem()
                                              .isNull()) {  // station system
    system = feature->getStation()->getCoordinateSystem();
  } else if (!feature->getCoordinateSystem().isNull()) {  // nominal system
    system = feature->getCoordinateSystem();
  }
  if (system.isNull()) {
    return;
  }

  // set active system
  system->setActiveCoordinateSystemState(true);
}

/*!
 * \brief FeatureTableModel::getActiveGroupName
 * \return
 */
const QString FeatureTableModel::getActiveGroupName() const {
  // check current job
  if (this->currentJob.isNull()) {
    return "";
  }

  return this->currentJob->getActiveGroup();
}

/*!
 * \brief FeatureTableModel::setActiveGroupName
 * \param group
 */
void FeatureTableModel::setActiveGroupName(const QString& group) {
  // check current job
  if (this->currentJob.isNull()) {
    return;
  }

  this->currentJob->setActiveGroup(group);
}

/*!
 * \brief FeatureTableModel::getActualNominalFilter
 * \return
 */
const ActualNominalFilter& FeatureTableModel::getActualNominalFilter() const {
  return this->actualNominalFilter;
}

/*!
 * \brief FeatureTableModel::setActualNominalFilter
 * \param filter
 */
void FeatureTableModel::setActualNominalFilter(
    const ActualNominalFilter& filter) {
  this->actualNominalFilter = filter;
  this->updateModel();
}

/*!
 * \brief FeatureTableModel::getCurrentJob
 * \return
 */
const oi::Guarded<OiJob>& FeatureTableModel::getCurrentJob() const {
  return this->currentJob;
}

/*!
 * \brief FeatureTableModel::setCurrentJob
 * \param job
 */
void FeatureTableModel::setCurrentJob(const oi::Guarded<OiJob>& job) {
  if (!job.isNull()) {
    // disconnect current job
    if (!this->currentJob.isNull()) {
      this->disconnectJob();
    }

    this->currentJob = job;
    this->connectJob();
    this->updateModel();
  }
}

/*!
 * \brief FeatureTableModel::getParameterDisplayConfig
 * \return
 */
const ParameterDisplayConfig& FeatureTableModel::getParameterDisplayConfig()
    const {
  return this->parameterDisplayConfig;
}

/*!
 * \brief FeatureTableModel::setParameterDisplayConfig
 * \param config
 */
void FeatureTableModel::setParameterDisplayConfig(
    const ParameterDisplayConfig& config) {
  this->parameterDisplayConfig = config;
  this->updateModel();
}

/*!
 * \brief FeatureTableModel::getMeasurementConfigManager
 * \return
 */
const QPointer<MeasurementConfigManager>&
FeatureTableModel::getMeasurementConfigManager() const {
  return this->measurementConfigManager;
}

/*!
 * \brief FeatureTableModel::setMeasurementConfigManager
 * \param mConfigManager
 */
void FeatureTableModel::setMeasurementConfigManager(
    const QPointer<MeasurementConfigManager>& mConfigManager) {
  this->measurementConfigManager = mConfigManager;
}

void FeatureTableModel::requestUpdateModel() { this->updateRequested = true; }

void FeatureTableModel::updateModelIfRequested() {
  if (this->updateRequested) {
    updateModel();
    this->updateRequested = false;
  }
}

/*!
 * \brief FeatureTableModel::updateModel
 */
void FeatureTableModel::updateModel() {
  emit this->layoutAboutToBeChanged();
  emit this->layoutChanged();
  emit this->sendMessage("Feature table updated",
                         MessageTypes::eInformationMessage,
                         MessageDestinations::eStatusBarMessage);
}

/*!
 * \brief FeatureTableModel::getDisplayValue
 * Returns the string that shall be displayed for the given feature at the
 * specified column
 * \param feature
 * \param column
 * \return
 */
QVariant FeatureTableModel::getDisplayValue(
    const oi::Guarded<FeatureWrapper>& feature, const int& column) const {
  // check if the column exists in available display attributes
  if (column < 0 || getFeatureDisplayAttributes().size() <= column) {
    return QVariant();
  }

  // get the display attribute
  int attr = getFeatureDisplayAttributes().at(column);

  if (getIsFeatureDisplayAttribute(attr)) {  // feature attributes

    switch ((FeatureDisplayAttributes)attr) {
      case eFeatureDisplayType:
        return feature->getFeature()->getDisplayType();
      case eFeatureDisplayIsActual:
        return feature->getFeature()->getDisplayIsActual();
      case eFeatureDisplayName:
        return feature->getFeature()->getFeatureName();
      case eFeatureDisplayComment:
        return feature->getFeature()->getComment();
      case eFeatureDisplayGroup:
        return feature->getFeature()->getGroupName();
      case eFeatureDisplayIsSolved:
        return QVariant();
      case eFeatureDisplayIsUpdated:
        return feature->getFeature()->getDisplayIsUpdated();
      case eFeatureDisplayFunctions:
        return feature->getFeature()->getDisplayFunctions();
      case eFeatureDisplayUsedFor:
        return feature->getFeature()->getDisplayUsedFor();
      case eFeatureDisplayPreviouslyNeeded:
        return feature->getFeature()->getDisplayPreviouslyNeeded();
      case eFeatureDisplayStDev:
        return feature->getFeature()->getDisplayStDev(
            this->parameterDisplayConfig.getDisplayUnit(DimensionType::eMetric),
            this->parameterDisplayConfig.getDisplayDigits(
                DimensionType::eMetric));
      case eFeatureDisplayMeasurementConfig:
        return feature->getFeature()->getDisplayMeasurementConfig();
      case eFeatureDisplayObservations:
        return feature->getFeature()->getDisplayObservations();
      case eFeatureDisplayX:
        return feature->getFeature()->getDisplayX(
            this->parameterDisplayConfig.getDisplayUnit(DimensionType::eMetric),
            this->parameterDisplayConfig.getDisplayDigits(
                DimensionType::eMetric));
      case eFeatureDisplayY:
        return feature->getFeature()->getDisplayY(
            this->parameterDisplayConfig.getDisplayUnit(DimensionType::eMetric),
            this->parameterDisplayConfig.getDisplayDigits(
                DimensionType::eMetric));
      case eFeatureDisplayZ:
        return feature->getFeature()->getDisplayZ(
            this->parameterDisplayConfig.getDisplayUnit(DimensionType::eMetric),
            this->parameterDisplayConfig.getDisplayDigits(
                DimensionType::eMetric));
      case eFeatureDisplayPrimaryI:
        return feature->getFeature()->getDisplayPrimaryI(
            this->parameterDisplayConfig.getDisplayDigits(
                DimensionType::eDimensionless));
      case eFeatureDisplayPrimaryJ:
        return feature->getFeature()->getDisplayPrimaryJ(
            this->parameterDisplayConfig.getDisplayDigits(
                DimensionType::eDimensionless));
      case eFeatureDisplayPrimaryK:
        return feature->getFeature()->getDisplayPrimaryK(
            this->parameterDisplayConfig.getDisplayDigits(
                DimensionType::eDimensionless));
      case eFeatureDisplayRadiusA:
        return feature->getFeature()->getDisplayRadiusA(
            this->parameterDisplayConfig.getDisplayUnit(DimensionType::eMetric),
            this->parameterDisplayConfig.getDisplayDigits(
                DimensionType::eMetric));
      case eFeatureDisplayRadiusB:
        return feature->getFeature()->getDisplayRadiusB(
            this->parameterDisplayConfig.getDisplayUnit(DimensionType::eMetric),
            this->parameterDisplayConfig.getDisplayDigits(
                DimensionType::eMetric));
      case eFeatureDisplaySecondaryI:
        return feature->getFeature()->getDisplaySecondaryI(
            this->parameterDisplayConfig.getDisplayDigits(
                DimensionType::eDimensionless));
      case eFeatureDisplaySecondaryJ:
        return feature->getFeature()->getDisplaySecondaryJ(
            this->parameterDisplayConfig.getDisplayDigits(
                DimensionType::eDimensionless));
      case eFeatureDisplaySecondaryK:
        return feature->getFeature()->getDisplaySecondaryK(
            this->parameterDisplayConfig.getDisplayDigits(
                DimensionType::eDimensionless));
      case eFeatureDisplayAperture:
        return feature->getFeature()->getDisplayAperture(
            this->parameterDisplayConfig.getDisplayUnit(
                DimensionType::eAngular),
            this->parameterDisplayConfig.getDisplayDigits(
                DimensionType::eAngular));
      case eFeatureDisplayA:
        return feature->getFeature()->getDisplayA(
            this->parameterDisplayConfig.getDisplayUnit(DimensionType::eMetric),
            this->parameterDisplayConfig.getDisplayDigits(
                DimensionType::eMetric));
      case eFeatureDisplayB:
        return feature->getFeature()->getDisplayB(
            this->parameterDisplayConfig.getDisplayUnit(DimensionType::eMetric),
            this->parameterDisplayConfig.getDisplayDigits(
                DimensionType::eMetric));
      case eFeatureDisplayC:
        return feature->getFeature()->getDisplayC(
            this->parameterDisplayConfig.getDisplayUnit(DimensionType::eMetric),
            this->parameterDisplayConfig.getDisplayDigits(
                DimensionType::eMetric));
      case eFeatureDisplayAngle:
        return feature->getFeature()->getDisplayAngle(
            this->parameterDisplayConfig.getDisplayUnit(
                DimensionType::eAngular),
            this->parameterDisplayConfig.getDisplayDigits(
                DimensionType::eAngular));
      case eFeatureDisplayDistance:
        return feature->getFeature()->getDisplayDistance(
            this->parameterDisplayConfig.getDisplayUnit(DimensionType::eMetric),
            this->parameterDisplayConfig.getDisplayDigits(
                DimensionType::eMetric));
      case eFeatureDisplayMeasurementSeries:
        return feature->getFeature()->getDisplayMeasurementSeries(
            this->parameterDisplayConfig.getDisplayUnit(
                DimensionType::eDimensionless),
            this->parameterDisplayConfig.getDisplayDigits(
                DimensionType::eDimensionless));
      case eFeatureDisplayTemperature:
        return feature->getFeature()->getDisplayTemperature(
            this->parameterDisplayConfig.getDisplayUnit(
                DimensionType::eTemperature),
            this->parameterDisplayConfig.getDisplayDigits(
                DimensionType::eTemperature));
      case eFeatureDisplayLength:
        return feature->getFeature()->getDisplayLength(
            this->parameterDisplayConfig.getDisplayUnit(DimensionType::eMetric),
            this->parameterDisplayConfig.getDisplayDigits(
                DimensionType::eMetric));
      case eFeatureDisplayExpansionOriginX:
        return feature->getFeature()->getDisplayExpansionOriginX(
            this->parameterDisplayConfig.getDisplayUnit(DimensionType::eMetric),
            this->parameterDisplayConfig.getDisplayDigits(
                DimensionType::eMetric));
      case eFeatureDisplayExpansionOriginY:
        return feature->getFeature()->getDisplayExpansionOriginY(
            this->parameterDisplayConfig.getDisplayUnit(DimensionType::eMetric),
            this->parameterDisplayConfig.getDisplayDigits(
                DimensionType::eMetric));
      case eFeatureDisplayExpansionOriginZ:
        return feature->getFeature()->getDisplayExpansionOriginZ(
            this->parameterDisplayConfig.getDisplayUnit(DimensionType::eMetric),
            this->parameterDisplayConfig.getDisplayDigits(
                DimensionType::eMetric));
      case eFeatureDisplayFormError:
        return feature->getFeature()->getDisplayFormError(
            this->parameterDisplayConfig.getDisplayUnit(DimensionType::eMetric),
            this->parameterDisplayConfig.getDisplayDigits(
                DimensionType::eMetric));
    }

  } else if (getIsTrafoParamDisplayAttribute(attr)) {  // trafo param attributes

    switch ((TrafoParamDisplayAttributes)attr) {
      case eTrafoParamDisplayStartSystem:
        return feature->getFeature()->getDisplayStartSystem();
      case eTrafoParamDisplayDestinationSystem:
        return feature->getFeature()->getDisplayDestinationSystem();
      case eTrafoParamDisplayTranslationX:
        return feature->getFeature()->getDisplayTranslationX(
            this->parameterDisplayConfig.getDisplayUnit(DimensionType::eMetric),
            this->parameterDisplayConfig.getDisplayDigits(
                DimensionType::eMetric));
      case eTrafoParamDisplayTranslationY:
        return feature->getFeature()->getDisplayTranslationY(
            this->parameterDisplayConfig.getDisplayUnit(DimensionType::eMetric),
            this->parameterDisplayConfig.getDisplayDigits(
                DimensionType::eMetric));
      case eTrafoParamDisplayTranslationZ:
        return feature->getFeature()->getDisplayTranslationZ(
            this->parameterDisplayConfig.getDisplayUnit(DimensionType::eMetric),
            this->parameterDisplayConfig.getDisplayDigits(
                DimensionType::eMetric));
      case eTrafoParamDisplayRotationX:
        return feature->getFeature()->getDisplayRotationX(
            this->parameterDisplayConfig.getDisplayUnit(
                DimensionType::eAngular),
            this->parameterDisplayConfig.getDisplayDigits(
                DimensionType::eAngular));
      case eTrafoParamDisplayRotationY:
        return feature->getFeature()->getDisplayRotationY(
            this->parameterDisplayConfig.getDisplayUnit(
                DimensionType::eAngular),
            this->parameterDisplayConfig.getDisplayDigits(
                DimensionType::eAngular));
      case eTrafoParamDisplayRotationZ:
        return feature->getFeature()->getDisplayRotationZ(
            this->parameterDisplayConfig.getDisplayUnit(
                DimensionType::eAngular),
            this->parameterDisplayConfig.getDisplayDigits(
                DimensionType::eAngular));
      case eTrafoParamDisplayScaleX:
        return feature->getFeature()->getDisplayScaleX(
            this->parameterDisplayConfig.getDisplayDigits(
                DimensionType::eDimensionless));
      case eTrafoParamDisplayScaleY:
        return feature->getFeature()->getDisplayScaleY(
            this->parameterDisplayConfig.getDisplayDigits(
                DimensionType::eDimensionless));
      case eTrafoParamDisplayScaleZ:
        return feature->getFeature()->getDisplayScaleZ(
            this->parameterDisplayConfig.getDisplayDigits(
                DimensionType::eDimensionless));
      case eTrafoParamDisplayIsUsed:
        return QVariant();
      case eTrafoParamDisplayValidTime:
        return feature->getFeature()->getDisplayValidTime();
      case eTrafoParamDisplayIsMovement:
        return feature->getFeature()->getDisplayIsMovement();
      case eTrafoParamDisplayIsDatumTransformation:
        return QVariant();
    }
  }
  return QVariant();
}

/*!
 * \brief FeatureTableModel::getBackgroundValue
 * \param feature
 * \param column
 * \return
 */
QVariant FeatureTableModel::getBackgroundValue(
    const oi::Guarded<FeatureWrapper>& feature, const int& column) const {
  // check if the column exists in available display attributes
  if (column < 0 || getFeatureDisplayAttributes().size() <= column) {
    return QVariant();
  }

  // active feature
  if (feature->getFeature()->getIsActiveFeature()) {
    return QColor(QColor::fromCmykF(0.59, 0.40, 0.10, 0.10).lighter());
  }

  // active station
  if (!feature->getStation().isNull() &&
      feature->getStation()->getIsActiveStation()) {
    return QColor(Qt::darkGray);
  }

  // inactive station
  if (!feature->getStation().isNull()) {
    return QColor(Qt::lightGray);
  }

  // get the display attribute
  int attr = getFeatureDisplayAttributes().at(column);

  // set background for parameter cells of unsolved features
  if (!feature->getFeature()->getIsSolved() &&
      getIsFeatureDisplayAttribute(attr)) {  // feature attributes

    switch ((FeatureDisplayAttributes)attr) {
      case eFeatureDisplayX:
        return QColor(Qt::yellow);
      case eFeatureDisplayY:
        return QColor(Qt::yellow);
      case eFeatureDisplayZ:
        return QColor(Qt::yellow);
      case eFeatureDisplayPrimaryI:
        return QColor(Qt::yellow);
      case eFeatureDisplayPrimaryJ:
        return QColor(Qt::yellow);
      case eFeatureDisplayPrimaryK:
        return QColor(Qt::yellow);
      case eFeatureDisplayRadiusA:
        return QColor(Qt::yellow);
      case eFeatureDisplayRadiusB:
        return QColor(Qt::yellow);
      case eFeatureDisplaySecondaryI:
        return QColor(Qt::yellow);
      case eFeatureDisplaySecondaryJ:
        return QColor(Qt::yellow);
      case eFeatureDisplaySecondaryK:
        return QColor(Qt::yellow);
      case eFeatureDisplayAperture:
        return QColor(Qt::yellow);
      case eFeatureDisplayA:
        return QColor(Qt::yellow);
      case eFeatureDisplayB:
        return QColor(Qt::yellow);
      case eFeatureDisplayC:
        return QColor(Qt::yellow);
      case eFeatureDisplayAngle:
        return QColor(Qt::yellow);
      case eFeatureDisplayDistance:
        return QColor(Qt::yellow);
      case eFeatureDisplayMeasurementSeries:
        return QColor(Qt::yellow);
      case eFeatureDisplayTemperature:
        return QColor(Qt::yellow);
      case eFeatureDisplayLength:
        return QColor(Qt::yellow);
      case eFeatureDisplayExpansionOriginX:
        return QColor(Qt::yellow);
      case eFeatureDisplayExpansionOriginY:
        return QColor(Qt::yellow);
      case eFeatureDisplayExpansionOriginZ:
        return QColor(Qt::yellow);
      case eFeatureDisplayFormError:
        return QColor(Qt::yellow);
    }

  } else if (!feature->getFeature()->getIsSolved() &&
             getIsTrafoParamDisplayAttribute(attr)) {  // trafo param attributes

    switch ((TrafoParamDisplayAttributes)attr) {
      case eTrafoParamDisplayTranslationX:
        return QColor(Qt::yellow);
      case eTrafoParamDisplayTranslationY:
        return QColor(Qt::yellow);
      case eTrafoParamDisplayTranslationZ:
        return QColor(Qt::yellow);
      case eTrafoParamDisplayRotationX:
        return QColor(Qt::yellow);
      case eTrafoParamDisplayRotationY:
        return QColor(Qt::yellow);
      case eTrafoParamDisplayRotationZ:
        return QColor(Qt::yellow);
      case eTrafoParamDisplayScaleX:
        return QColor(Qt::yellow);
      case eTrafoParamDisplayScaleY:
        return QColor(Qt::yellow);
      case eTrafoParamDisplayScaleZ:
        return QColor(Qt::yellow);
    }
  }

  // nominals
  if (!feature->getGeometry().isNull() &&
      !feature->getGeometry()->getOwnerDescription().has_value()) {
    return QColor(QColor::fromRgb(230, 230, 180));
  }

  return QVariant();
}

/*!
 * \brief FeatureTableModel::getForegroundValue
 * Picks a readable text color for cells with a custom background (e.g.
 * the default theme's light text is unreadable on the yellow "unsolved"
 * highlight), falling back to the theme's default text color otherwise.
 * \param feature
 * \param column
 * \return
 */
QVariant FeatureTableModel::getForegroundValue(
    const oi::Guarded<FeatureWrapper>& feature, const int& column) const {
  QVariant background = this->getBackgroundValue(feature, column);
  if (!background.canConvert<QColor>()) {
    return QVariant();
  }

  QColor backgroundColor = background.value<QColor>();
  qreal luminance = 0.299 * backgroundColor.redF() +
                    0.587 * backgroundColor.greenF() +
                    0.114 * backgroundColor.blueF();
  return luminance > 0.5 ? QColor(Qt::black) : QColor(Qt::white);
}

/*!
 * \brief FeatureTableModel::connectJob
 */
void FeatureTableModel::connectJob() {
  QObject::connect(QtJobAdapter::of(this->currentJob.data()),
                   &QtJobAdapter::featureSetChanged, this,
                   &FeatureTableModel::requestUpdateModel, Qt::AutoConnection);
  QObject::connect(QtJobAdapter::of(this->currentJob.data()),
                   &QtJobAdapter::activeCoordinateSystemChanged, this,
                   &FeatureTableModel::requestUpdateModel, Qt::AutoConnection);
  QObject::connect(QtJobAdapter::of(this->currentJob.data()),
                   &QtJobAdapter::activeFeatureChanged, this,
                   &FeatureTableModel::requestUpdateModel, Qt::AutoConnection);
  QObject::connect(QtJobAdapter::of(this->currentJob.data()),
                   &QtJobAdapter::activeStationChanged, this,
                   &FeatureTableModel::requestUpdateModel, Qt::AutoConnection);
  QObject::connect(QtJobAdapter::of(this->currentJob.data()),
                   &QtJobAdapter::featureAttributesChanged, this,
                   &FeatureTableModel::requestUpdateModel, Qt::AutoConnection);
  QObject::connect(QtJobAdapter::of(this->currentJob.data()),
                   &QtJobAdapter::featureRecalculated, this,
                   &FeatureTableModel::requestUpdateModel, Qt::AutoConnection);
  QObject::connect(QtJobAdapter::of(this->currentJob.data()),
                   &QtJobAdapter::featuresRecalculated, this,
                   &FeatureTableModel::requestUpdateModel, Qt::AutoConnection);
  QObject::connect(QtJobAdapter::of(this->currentJob.data()),
                   &QtJobAdapter::geometryMeasurementConfigChanged, this,
                   &FeatureTableModel::requestUpdateModel, Qt::AutoConnection);
  QObject::connect(QtJobAdapter::of(this->currentJob.data()),
                   &QtJobAdapter::activeGroupChanged, this,
                   &FeatureTableModel::requestUpdateModel, Qt::AutoConnection);
  QObject::connect(QtJobAdapter::of(this->currentJob.data()),
                   &QtJobAdapter::geometryIsCommonChanged, this,
                   &FeatureTableModel::requestUpdateModel, Qt::AutoConnection);
  QObject::connect(QtJobAdapter::of(this->currentJob.data()),
                   &QtJobAdapter::trafoParamIsDatumChanged, this,
                   &FeatureTableModel::requestUpdateModel, Qt::AutoConnection);

  connectToJob(this, &FeatureTableModel::sendMessage, this->currentJob,
               &OiJob::sendMessage);
}

/*!
 * \brief FeatureTableModel::disconnectJob
 */
void FeatureTableModel::disconnectJob() {
  QObject::disconnect(QtJobAdapter::of(this->currentJob.data()),
                      &QtJobAdapter::featureSetChanged, this,
                      &FeatureTableModel::requestUpdateModel);
  QObject::disconnect(QtJobAdapter::of(this->currentJob.data()),
                      &QtJobAdapter::activeCoordinateSystemChanged, this,
                      &FeatureTableModel::requestUpdateModel);
  QObject::disconnect(QtJobAdapter::of(this->currentJob.data()),
                      &QtJobAdapter::activeFeatureChanged, this,
                      &FeatureTableModel::requestUpdateModel);
  QObject::disconnect(QtJobAdapter::of(this->currentJob.data()),
                      &QtJobAdapter::activeStationChanged, this,
                      &FeatureTableModel::requestUpdateModel);
  QObject::disconnect(QtJobAdapter::of(this->currentJob.data()),
                      &QtJobAdapter::featureAttributesChanged, this,
                      &FeatureTableModel::requestUpdateModel);
  QObject::disconnect(QtJobAdapter::of(this->currentJob.data()),
                      &QtJobAdapter::featureRecalculated, this,
                      &FeatureTableModel::requestUpdateModel);
  QObject::disconnect(QtJobAdapter::of(this->currentJob.data()),
                      &QtJobAdapter::featuresRecalculated, this,
                      &FeatureTableModel::requestUpdateModel);
  QObject::disconnect(QtJobAdapter::of(this->currentJob.data()),
                      &QtJobAdapter::geometryMeasurementConfigChanged, this,
                      &FeatureTableModel::requestUpdateModel);
  QObject::disconnect(QtJobAdapter::of(this->currentJob.data()),
                      &QtJobAdapter::activeGroupChanged, this,
                      &FeatureTableModel::requestUpdateModel);
  QObject::disconnect(QtJobAdapter::of(this->currentJob.data()),
                      &QtJobAdapter::geometryIsCommonChanged, this,
                      &FeatureTableModel::requestUpdateModel);
  QObject::disconnect(QtJobAdapter::of(this->currentJob.data()),
                      &QtJobAdapter::trafoParamIsDatumChanged, this,
                      &FeatureTableModel::requestUpdateModel);

  disconnectFromJob(this);
}

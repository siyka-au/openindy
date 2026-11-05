#ifndef MEASUREMENTCONFIGURATIONPROXYMODEL_H
#define MEASUREMENTCONFIGURATIONPROXYMODEL_H

#include <QPointer>
#include <QSortFilterProxyModel>

#include "measurementconfigurationmodel.h"
#include "sensortype.h"
#include "types.h"
#include "util.h"

#if defined(OI_MAIN_LIB)
#define OI_MAIN_EXPORT Q_DECL_EXPORT
#else
#define OI_MAIN_EXPORT Q_DECL_IMPORT
#endif

using namespace oi;

enum MeasurementConfigurationFilterTypes {
  eNo_MeasurementConfigurationFilter = 0,
  eAll_MeasurementConfigurationFilter,
  eProject_MeasurementConfigurationFilter,
  eUser_MeasurementConfigurationFilter,
  eCreateFeature_MeasurementConfigurationFilter,
};

/*!
 * \brief The MeasurementConfigurationProxyModel class
 * Model that holds all available measurement configurations filtered by saved
 * and project property
 */
class OI_MAIN_EXPORT MeasurementConfigurationProxyModel
    : public QSortFilterProxyModel {
  Q_OBJECT

 public:
  explicit MeasurementConfigurationProxyModel(QObject* parent = 0);

  // ##########
  // set filter
  // ##########

  void setFilter(const bool& showAll);
  //! activeSensorType is a convenience narrowing, not the safety net - a
  //! config whose own tag doesn't match is still just filtered out of this
  //! UI list; Controller::_startMeasurement is what actually rejects an
  //! incompatible config reaching the sensor. Default (invalid) means "no
  //! sensor known yet" and shows every type, matching prior behaviour.
  void setFilter(const QList<ElementTypes> neededElements,
                 FeatureTypes typeOfFeature, QList<FeatureTypes> applicableFor,
                 const SensorType& activeSensorType = SensorType());
  void setFilterProjectConfig();
  void setFilterUserConfig();
  void resetFilter();

  // ###########################################
  // override methods of sort filter proxy model
  // ###########################################

  QVariant headerData(int section, Qt::Orientation orientation, int role) const;

  MeasurementConfig getMeasurementConfig(const QModelIndex& index) const;

 signals:

  // #########################
  // send messages to OpenIndy
  // #########################

  void sendMessage(const QString& msg, const MessageTypes& msgType,
                   const MessageDestinations& msgDest = eConsoleMessage);

 protected:
  bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const;
  bool filterAcceptsColumn(int source_column,
                           const QModelIndex& source_parent) const;
  bool lessThan(const QModelIndex& left,
                const QModelIndex& right) const override;
  QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;

 private:
  // ################
  // filter attribute
  // ################

  MeasurementConfigurationFilterTypes filterType;
  QList<ElementTypes> neededElements;
  FeatureTypes typeOfFeature;
  QList<FeatureTypes> functionIsApplicableFor;
  SensorType activeSensorType;
};

#endif  // MEASUREMENTCONFIGURATIONPROXYMODEL_H

#ifndef FEATURETABLEMODEL_H
#define FEATURETABLEMODEL_H

#include <QAbstractTableModel>
#include <QColor>
#include <QMessageBox>
#include <QObject>
#include <QPointer>
#include <atomic>

#include "console.h"
#include "measurementconfigmanager.h"
#include "oijob.h"
#include "parameterdisplayconfig.h"
#include "pluginloader.h"
#include "pluginrepository.h"
#include "types.h"
#include "util.h"

using namespace oi;

/*!
 * \brief The FeatureTableModel class
 * Model with all features of an OpenIndy job
 */
class FeatureTableModel : public QAbstractTableModel {
  Q_OBJECT

 public:
  explicit FeatureTableModel(const oi::Guarded<OiJob>& job,
                             QObject* parent = nullptr);
  explicit FeatureTableModel(QObject* parent = nullptr);

  enum class EditMode {  // bit flags
    eDefault = 0,        // do nothing
    eFunctionCopyScalarInputParams = 1 << 0,
    eFunctionCopyUsedElements = 1 << 1,
  };
  Q_DECLARE_FLAGS(EditModes, EditMode)

  // ########################################
  // override methods of abstract table model
  // ########################################

  [[nodiscard]] int rowCount(
      const QModelIndex& parent = QModelIndex()) const override;
  [[nodiscard]] int columnCount(
      const QModelIndex& parent = QModelIndex()) const override;

  [[nodiscard]] QVariant data(const QModelIndex& index,
                              int role = Qt::DisplayRole) const override;

  [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation,
                                    int role = Qt::DisplayRole) const override;

  [[nodiscard]] Qt::ItemFlags flags(const QModelIndex& index) const override;

  bool setData(const QModelIndex& index, const QVariant& value,
               int role = Qt::EditRole) override;
  // extra parameter
  bool setData(const QModelIndex& index, const QVariant& value, int role,
               EditModes editMode);

  // ##############################
  // get id of the feature at index
  // ##############################

  int getFeatureIdAtIndex(const QModelIndex& index);

  // #########################
  // get or set active feature
  // #########################

  [[nodiscard]] oi::Guarded<FeatureWrapper> getActiveFeature() const;
  void setActiveFeature(const QModelIndex& index);
  void setActiveFeature(const int& id);

  [[nodiscard]] oi::Guarded<Station> getActiveStation() const;
  void setActiveStation(const QModelIndex& index);
  void setActiveStation(const int& id);

  [[nodiscard]] oi::Guarded<CoordinateSystem> getActiveCoordinateSystem() const;
  void setActiveCoordinateSystem(const QString& name);

  // #######################
  // get or set active group
  // #######################

  [[nodiscard]] const QString getActiveGroupName() const;
  void setActiveGroupName(const QString& group);

  // ################################
  // get or set actual nominal filter
  // ################################

  [[nodiscard]] const ActualNominalFilter& getActualNominalFilter() const;
  void setActualNominalFilter(const ActualNominalFilter& filter);

  // ###############################
  // get or set current OpenIndy job
  // ###############################

  [[nodiscard]] const oi::Guarded<OiJob>& getCurrentJob() const;
  void setCurrentJob(const oi::Guarded<OiJob>& job);

  // #########################
  // get or set display config
  // #########################

  [[nodiscard]] const ParameterDisplayConfig& getParameterDisplayConfig() const;
  void setParameterDisplayConfig(const ParameterDisplayConfig& config);

  // #########################
  // get or set config manager
  // #########################

  [[nodiscard]] const QPointer<MeasurementConfigManager>&
  getMeasurementConfigManager() const;
  void setMeasurementConfigManager(
      const QPointer<MeasurementConfigManager>& mConfigManager);

 signals:

  // #########################
  // send messages to OpenIndy
  // #########################

  void sendMessage(const QString& msg, const MessageTypes& msgType,
                   const MessageDestinations& msgDest =
                       MessageDestinations::eConsoleMessage);

  void recalcActiveFeature();

 private slots:

  // ##############################################################
  // update the model when job state or display config have changed
  // ##############################################################
  void requestUpdateModel();
  void updateModelIfRequested();
  void updateModel();

 private:
  // ##############
  // helper methods
  // ##############

  [[nodiscard]] QVariant getDisplayValue(
      const oi::Guarded<FeatureWrapper>& feature, const int& column) const;
  [[nodiscard]] QVariant getBackgroundValue(
      const oi::Guarded<FeatureWrapper>& feature, const int& column) const;
  [[nodiscard]] QVariant getForegroundValue(
      const oi::Guarded<FeatureWrapper>& feature, const int& column) const;

  void connectJob();
  void disconnectJob();

  oi::Guarded<Function> loadFunctionByName(QString name);

 private:
  // ###########
  // current job
  // ###########

  oi::Guarded<OiJob> currentJob;

  // ##############
  // display config
  // ##############

  ParameterDisplayConfig parameterDisplayConfig;

  // ##############
  // config manager
  // ##############

  QPointer<MeasurementConfigManager> measurementConfigManager;

  // #################
  // helper attributes
  // #################

  ActualNominalFilter actualNominalFilter;

  std::atomic<bool> updateRequested;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(FeatureTableModel::EditModes)

#endif  // FEATURETABLEMODEL_H

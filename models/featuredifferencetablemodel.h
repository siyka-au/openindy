#ifndef FEATUREDIFFERENCETABLEMODEL_H
#define FEATUREDIFFERENCETABLEMODEL_H

#include <QAbstractTableModel>
#include <QColor>
#include <QObject>
#include <QPointer>

#include "oijob.h"
#include "parameterdisplayconfig.h"
#include "types.h"
#include "util.h"

using namespace oi;

/*!
 * \brief The FeatureDifferenceTableModel class
 */
class FeatureDifferenceTableModel : public QAbstractTableModel {
  Q_OBJECT

 public:
  explicit FeatureDifferenceTableModel(const oi::Guarded<OiJob>& job,
                                       QObject* parent = nullptr);
  explicit FeatureDifferenceTableModel(QObject* parent = nullptr);

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

  // ######################
  // set tolerance for view
  // ######################
  void setTolerance(double tolerance);

 signals:

  // #########################
  // send messages to OpenIndy
  // #########################

  void sendMessage(const QString& msg, const MessageTypes& msgType,
                   const MessageDestinations& msgDest =
                       MessageDestinations::eConsoleMessage);

 private slots:

  // ##############################################################
  // update the model when job state or display config have changed
  // ##############################################################

  void updateModel();

 private:
  // ###########
  // current job
  // ###########
  void connectJob();
  void disconnectJob();

  [[nodiscard]] QPair<bool, double> getDifferenceAsDisplayUnit(
      oi::Guarded<FeatureWrapper> feature, const QModelIndex index) const;

  oi::Guarded<OiJob> currentJob;

  // ##############
  // display config
  // ##############

  ParameterDisplayConfig parameterDisplayConfig;

  double tolerance;
};

#endif  // FEATUREDIFFERENCETABLEMODEL_H

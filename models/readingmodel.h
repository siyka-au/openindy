#ifndef READINGMODEL_H
#define READINGMODEL_H

#include <QAbstractTableModel>

#include "oijob.h"
#include "parameterdisplayconfig.h"
#include "types.h"
#include "util.h"

using namespace oi;

/*!
 * \brief The ReadingModel class
 */
class ReadingModel : public QAbstractTableModel {
  Q_OBJECT

 public:
  explicit ReadingModel(QObject* parent = nullptr);

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

  void activeFeatureChanged();
  void geometryObservationsChanged(const int& featureId);

 private:
  // ##############
  // helper methods
  // ##############

  void updateModel();
  void connectJob();
  void disconnectJob();

 private:
  // ###########
  // current job
  // ###########

  oi::Guarded<OiJob> currentJob;

  // ##############
  // display config
  // ##############

  ParameterDisplayConfig parameterDisplayConfig;
};

#endif  // READINGMODEL_H

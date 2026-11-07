#ifndef BUNDLESTATIONSMODEL_H
#define BUNDLESTATIONSMODEL_H

#include <QAbstractItemModel>
#include <QAbstractTableModel>
#include <QJsonArray>
#include <QJsonObject>
#include <QObject>

#include "bundlestationitem.h"
#include "oijob.h"

/*!
 * \brief The BundleStationsModel class
 * Model with all bundle stations
 */
class BundleStationsModel : public QAbstractItemModel {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(BundleStationsModel)

 public:
  explicit BundleStationsModel(QObject* parent = nullptr);
  ~BundleStationsModel() override;

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

  [[nodiscard]] QModelIndex index(
      int row, int column,
      const QModelIndex& parent = QModelIndex()) const override;
  [[nodiscard]] QModelIndex parent(const QModelIndex& index) const override;

  // ###############
  // getter / setter
  // ###############

  // job
  [[nodiscard]] const oi::Guarded<oi::OiJob>& getCurrentJob() const;
  void setCurrentJob(const oi::Guarded<oi::OiJob>& job);

  // stations
  const QJsonArray& getStations();
  void setStations(const QJsonArray& stations);

  // template
  const QJsonObject& getBundleTemplate();
  void setBundleTemplate(const QJsonObject& bundleTemplate);

 signals:

  // #####################################
  // signals to inform about model changes
  // #####################################

  void stationsChanged();

 private slots:

  // ################
  // update the model
  // ################

  void updateModel();

 private:
  // ##############
  // helper methods
  // ##############

  void connectJob();
  void disconnectJob();

  // update json from current selection
  void updateJson();

  // #################
  // helper attributes
  // #################

  // root item and job
  BundleStationItem* rootItem;
  oi::Guarded<oi::OiJob> currentJob;

  // used stations and template
  QJsonObject bundleTemplate;
  QJsonArray usedStations;
};

#endif  // BUNDLESTATIONSMODEL_H

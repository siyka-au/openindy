#ifndef FUNCTIONWEIGHTSTABLEMODEL_H
#define FUNCTIONWEIGHTSTABLEMODEL_H

#include <QAbstractTableModel>
#include <QList>
#include <QObject>
#include <QPointer>

#include "feature.h"
#include "oijob.h"

using namespace oi;

/*!
 * \brief The FunctionWeightsTableModel class
 * model the weights of function input elements
 */
class FunctionWeightsTableModel : public QAbstractTableModel {
  Q_OBJECT
 public:
  explicit FunctionWeightsTableModel(QPointer<OiJob> job,
                                     QObject* parent = nullptr);
  explicit FunctionWeightsTableModel(QObject* parent = nullptr);

  // ###############################
  // get or set current OpenIndy job
  // ###############################

  const QPointer<OiJob>& getCurrentJob() const;
  void setCurrentJob(const QPointer<OiJob>& job);

  int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  int columnCount(const QModelIndex& parent = QModelIndex()) const override;

  QVariant data(const QModelIndex& index,
                int role = Qt::DisplayRole) const override;

  QVariant headerData(int section, Qt::Orientation orientation,
                      int role = Qt::DisplayRole) const override;

  Qt::ItemFlags flags(const QModelIndex& index) const override;

  bool setData(const QModelIndex& index, const QVariant& value,
               int role = Qt::EditRole) override;

 public slots:

  void setFunctionPosition(int pos);

  // ##############################################
  // update the model when changes
  // ##############################################

  void updateModel();

 private:
  QPointer<OiJob> currentJob;

  int functionPosition;

  void connectJob();
  void disconnectJob();

  void featureFunctionChanged();
};

#endif  // FUNCTIONWEIGHTSTABLEMODEL_H

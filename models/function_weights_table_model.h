#ifndef OI_APP_MODELS_FUNCTION_WEIGHTS_TABLE_MODEL_H_
#define OI_APP_MODELS_FUNCTION_WEIGHTS_TABLE_MODEL_H_

#include <QAbstractTableModel>
#include <QList>
#include <QObject>
#include <QPointer>

#include "feature.h"
#include "oi_job.h"

using namespace oi;

/*!
 * \brief The FunctionWeightsTableModel class
 * model the weights of function input elements
 */
class FunctionWeightsTableModel : public QAbstractTableModel {
  Q_OBJECT
 public:
  explicit FunctionWeightsTableModel(oi::Guarded<OiJob> job,
                                     QObject* parent = nullptr);
  explicit FunctionWeightsTableModel(QObject* parent = nullptr);

  // ###############################
  // get or set current OpenIndy job
  // ###############################

  [[nodiscard]] const oi::Guarded<OiJob>& getCurrentJob() const;
  void setCurrentJob(const oi::Guarded<OiJob>& job);

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

 public slots:

  void setFunctionPosition(int pos);

  // ##############################################
  // update the model when changes
  // ##############################################

  void updateModel();

 private:
  oi::Guarded<OiJob> currentJob;

  int functionPosition;

  void connectJob();
  void disconnectJob();

  void featureFunctionChanged();
};

#endif  // OI_APP_MODELS_FUNCTION_WEIGHTS_TABLE_MODEL_H_

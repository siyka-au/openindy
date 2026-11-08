#ifndef OI_APP_MODELS_USED_ELEMENTS_MODEL_H_
#define OI_APP_MODELS_USED_ELEMENTS_MODEL_H_

#include <QAbstractListModel>
#include <QList>
#include <QModelIndex>
#include <QPixmap>
#include <QPointer>

#include "feature_tree_item.h"
#include "function.h"
#include "oi_job.h"
#include "types.h"
#include "util.h"

using namespace oi;

/*!
 * \brief The UsedElementsModel class
 * Model that holds all used elements of a function of the active feature at a
 * special position
 */
class UsedElementsModel : public QAbstractListModel {
  Q_OBJECT

 public:
  explicit UsedElementsModel(QObject* parent = nullptr);

  // #######################################
  // override methods of abstract item model
  // #######################################

  [[nodiscard]] int rowCount(
      const QModelIndex& parent = QModelIndex()) const override;
  [[nodiscard]] int columnCount(
      const QModelIndex& parent = QModelIndex()) const override;

  [[nodiscard]] QVariant data(const QModelIndex& index,
                              int role = Qt::DisplayRole) const override;

  [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation,
                                    int role = Qt::DisplayRole) const override;

  // ####################
  // remove used elements
  // ####################

  void removeUsedElements(const QModelIndexList& selection);

  // ###############################
  // get or set current OpenIndy job
  // ###############################

  [[nodiscard]] const oi::Guarded<OiJob>& getCurrentJob() const;
  void setCurrentJob(const oi::Guarded<OiJob>& job);

  // ##########
  // set filter
  // ##########

  void setSelectedFunctionPosition(const int& functionPosition,
                                   const int& neededElementIndex);

 signals:

  // ###########################
  // signals to change job state
  // ###########################

  void startRemoveUsedElements(const oi::Guarded<FeatureWrapper>& target,
                               const int& functionPosition,
                               const int& neededElementsIndex,
                               const int& elementId);

 private slots:

  // ###########################################
  // update the model when job state has changed
  // ###########################################

  void updateModel();

 private:
  // ##############
  // helper methods
  // ##############

  void connectJob();
  void disconnectJob();

  void resetSelectedFunctionPosition();

  // ###########
  // current job
  // ###########

  oi::Guarded<OiJob> currentJob;

  // #################
  // function position
  // #################

  int functionPosition{};    // specifies the index of the function list of the
                             // active feature
  int neededElementIndex{};  // specifies the needed element index of the
                             // function at functionPosition

  // #######################
  // save connected function
  // #######################

  oi::Guarded<Function> connectedFunction;
};

#endif  // OI_APP_MODELS_USED_ELEMENTS_MODEL_H_

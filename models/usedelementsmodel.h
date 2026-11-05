#ifndef USEDELEMENTSMODEL_H
#define USEDELEMENTSMODEL_H

#include <QAbstractListModel>
#include <QList>
#include <QModelIndex>
#include <QPixmap>
#include <QPointer>

#include "featuretreeitem.h"
#include "function.h"
#include "oijob.h"
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

  [[nodiscard]] const QPointer<OiJob>& getCurrentJob() const;
  void setCurrentJob(const QPointer<OiJob>& job);

  // ##########
  // set filter
  // ##########

  void setSelectedFunctionPosition(const int& functionPosition,
                                   const int& neededElementIndex);

 signals:

  // ###########################
  // signals to change job state
  // ###########################

  void startRemoveUsedElements(const QPointer<FeatureWrapper>& target,
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

  QPointer<OiJob> currentJob;

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

  QPointer<Function> connectedFunction;
};

#endif  // USEDELEMENTSMODEL_H

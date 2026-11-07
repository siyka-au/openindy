#ifndef ACTIVEFEATUREFUNCTIONSMODEL_H
#define ACTIVEFEATUREFUNCTIONSMODEL_H

#include <QAbstractItemModel>
#include <QModelIndex>
#include <QPixmap>
#include <QPointer>

#include "function.h"
#include "functiontreeitem.h"
#include "oijob.h"
#include "types.h"
#include "util.h"

using namespace oi;

/*!
 * \brief The ActiveFeatureFunctionsModel class
 * Model that holds all functions of the active feature in a tree hierarchy
 */
class ActiveFeatureFunctionsModel : public QAbstractItemModel {
  Q_OBJECT

 public:
  explicit ActiveFeatureFunctionsModel(oi::Guarded<OiJob> job,
                                       QObject* parent = nullptr);
  explicit ActiveFeatureFunctionsModel(QObject* parent = nullptr);

  // #######################################
  // override methods of abstract item model
  // #######################################

  [[nodiscard]] int rowCount(
      const QModelIndex& parent = QModelIndex()) const override;
  [[nodiscard]] int columnCount(
      const QModelIndex& parent = QModelIndex()) const override;

  [[nodiscard]] QVariant data(const QModelIndex& index,
                              int role = Qt::DisplayRole) const override;

  [[nodiscard]] QModelIndex index(
      int row, int column,
      const QModelIndex& parent = QModelIndex()) const override;
  [[nodiscard]] QModelIndex parent(const QModelIndex& index) const override;

  [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation,
                                    int role) const override;

  // ###############################
  // get or set function information
  // ###############################

  // remove functions
  [[nodiscard]] bool canRemoveFunction(const QModelIndex& index) const;
  void removeFunction(const QModelIndex& index);

  // get description of function and input elements
  [[nodiscard]] QString getFunctionDescription(const QModelIndex& index) const;
  [[nodiscard]] QString getInputElementDescription(
      const QModelIndex& index) const;

  // check if the function supports a weighted alignment
  [[nodiscard]] bool getSupportsWeights(const QModelIndex& index) const;

  // get user defined scalar input parameters
  [[nodiscard]] QMap<QString, int> getIntegerParameter(
      const QModelIndex& index) const;
  [[nodiscard]] QMap<QString, double> getDoubleParameter(
      const QModelIndex& index) const;
  [[nodiscard]] QMap<QString, QString> getStringParameterSelection(
      const QModelIndex& index) const;

  // set scalar input parameters
  void setScalarInputParameters(const QModelIndex& index,
                                const QMap<QString, int>& intParams,
                                const QMap<QString, double>& doubleParams,
                                const QMap<QString, QString>& stringParams);

  // get all available string parameters
  [[nodiscard]] QMultiMap<QString, QString> getStringParameter(
      const QModelIndex& index) const;

  // ###############################
  // get or set current OpenIndy job
  // ###############################

  [[nodiscard]] const oi::Guarded<OiJob>& getCurrentJob() const;
  void setCurrentJob(const oi::Guarded<OiJob>& job);

  // #############
  // get root item
  // #############

  [[nodiscard]] const QPointer<FunctionTreeItem>& getRootItem() const;

 signals:

  // ###########################
  // signals to change job state
  // ###########################

  void startRemoveFunction(const int& functionIndex);

  // #########################
  // send messages to OpenIndy
  // #########################

  void sendMessage(const QString& msg, const MessageTypes& msgType,
                   const MessageDestinations& msgDest =
                       MessageDestinations::eConsoleMessage);

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

  void addGeometries();

  // ###########
  // current job
  // ###########

  oi::Guarded<OiJob> currentJob;

  // ##############
  // root tree item
  // ##############

  QPointer<FunctionTreeItem> rootItem;
};

#endif  // ACTIVEFEATUREFUNCTIONSMODEL_H

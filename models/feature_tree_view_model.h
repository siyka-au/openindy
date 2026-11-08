#ifndef OI_APP_MODELS_FEATURE_TREE_VIEW_MODEL_H_
#define OI_APP_MODELS_FEATURE_TREE_VIEW_MODEL_H_

#include <QAbstractItemModel>
#include <QModelIndex>
#include <QPixmap>
#include <QPointer>

#include "feature_tree_item.h"
#include "oi_job.h"
#include "types.h"
#include "util.h"

using namespace oi;

/*!
 * \brief The FeatureTreeViewModel class
 * Model that holds all features of an OpenIndy job in a tree structure
 */
class FeatureTreeViewModel : public QAbstractItemModel {
  Q_OBJECT

 public:
  explicit FeatureTreeViewModel(oi::Guarded<OiJob> job,
                                QObject* parent = nullptr);
  explicit FeatureTreeViewModel(QObject* parent = nullptr);

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

  // ###############################
  // get or set current OpenIndy job
  // ###############################

  [[nodiscard]] const oi::Guarded<OiJob>& getCurrentJob() const;
  void setCurrentJob(const oi::Guarded<OiJob>& job);

  // #############
  // get root item
  // #############

  [[nodiscard]] const QPointer<FeatureTreeItem>& getRootItem() const;

 signals:

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

  QPointer<FeatureTreeItem> rootItem;
};

#endif  // OI_APP_MODELS_FEATURE_TREE_VIEW_MODEL_H_

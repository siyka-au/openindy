#ifndef OI_APP_UI_SELECTIONMODELS_MULTI_SELECTION_MODEL_H_
#define OI_APP_UI_SELECTIONMODELS_MULTI_SELECTION_MODEL_H_

#include <QAbstractItemModel>
#include <QDebug>
#include <QItemSelectionModel>

class MultiSelectionModel : public QItemSelectionModel {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(MultiSelectionModel)

 public:
  explicit MultiSelectionModel(QAbstractItemModel* myModel);
  ~MultiSelectionModel() override;

 public slots:
  void select(const QItemSelection& selection, SelectionFlags command) override;
  void select(const QModelIndex& index,
              QItemSelectionModel::SelectionFlags command) override;

 private:
  bool checkIndex(const QModelIndex& index, const QItemSelection& selection);
};

#endif  // OI_APP_UI_SELECTIONMODELS_MULTI_SELECTION_MODEL_H_

#ifndef MULTISELECTIONMODEL_H
#define MULTISELECTIONMODEL_H

#include <QAbstractItemModel>
#include <QDebug>
#include <QItemSelectionModel>

class MultiSelectionModel : public QItemSelectionModel {
  Q_OBJECT

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

#endif  // MULTISELECTIONMODEL_H

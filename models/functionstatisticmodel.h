#ifndef FUNCTIONSTATISTICMODEL_H
#define FUNCTIONSTATISTICMODEL_H

#include <QAbstractTableModel>
#include <QPointer>

#include "feature.h"
#include "function.h"
#include "parameterdisplayconfig.h"
#include "statistic.h"
#include "types.h"
#include "util.h"

using namespace oi;

/*!
 * \brief The FunctionStatisticModel class
 * Model that holds the statistic of a specific function of a feature
 */
class FunctionStatisticModel : public QAbstractTableModel {
 public:
  explicit FunctionStatisticModel(const QPointer<Feature>& feature,
                                  QObject* parent = nullptr);

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

  // ##############################
  // get or set the current feature
  // ##############################

  [[nodiscard]] const QPointer<Feature>& getFeature() const;
  void setFeature(const QPointer<Feature>& feature);

  // #####################################
  // get or set the current function index
  // #####################################

  [[nodiscard]] const int& getFunctionIndex() const;
  void setFunctionIndex(const int& index);

  // #########################
  // get or set display config
  // #########################

  [[nodiscard]] const ParameterDisplayConfig& getParameterDisplayConfig() const;
  void setParameterDisplayConfig(const ParameterDisplayConfig& config);

 private slots:

  // ##############
  // helper methods
  // ##############

  void updateModel();

 private:
  // ##################################
  // current feature and function index
  // ##################################

  QPointer<Feature> feature;
  int functionIndex{-1};

  // ##############
  // display config
  // ##############

  ParameterDisplayConfig parameterDisplayConfig;
};

#endif  // FUNCTIONSTATISTICMODEL_H

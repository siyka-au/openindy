#ifndef OI_APP_MODELS_FUNCTION_STATISTIC_MODEL_H_
#define OI_APP_MODELS_FUNCTION_STATISTIC_MODEL_H_

#include <QAbstractTableModel>
#include <QPointer>

#include "feature.h"
#include "function.h"
#include "parameter_display_config.h"
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
  explicit FunctionStatisticModel(const oi::Guarded<Feature>& feature,
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

  [[nodiscard]] const oi::Guarded<Feature>& getFeature() const;
  void setFeature(const oi::Guarded<Feature>& feature);

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

  oi::Guarded<Feature> feature;
  int functionIndex{-1};

  // ##############
  // display config
  // ##############

  ParameterDisplayConfig parameterDisplayConfig;
};

#endif  // OI_APP_MODELS_FUNCTION_STATISTIC_MODEL_H_

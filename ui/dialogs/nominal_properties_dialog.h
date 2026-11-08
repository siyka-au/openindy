#ifndef OI_APP_UI_DIALOGS_NOMINAL_PROPERTIES_DIALOG_H_
#define OI_APP_UI_DIALOGS_NOMINAL_PROPERTIES_DIALOG_H_

#include <QDialog>
#include <QMap>
#include <QPointer>
#include <QShowEvent>
#include <map>

#include "model_manager.h"

using namespace oi;

namespace Ui {
class NominalPropertiesDialog;
}

/*!
 * \brief The NominalPropertiesDialog class
 */
class NominalPropertiesDialog : public QDialog {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(NominalPropertiesDialog)

 public:
  explicit NominalPropertiesDialog(QWidget* parent = nullptr);
  ~NominalPropertiesDialog() override;

  // ########################################################
  // get or set information about the current nominal feature
  // ########################################################

  [[nodiscard]] const int& getId() const;
  void setId(const int& id);

  [[nodiscard]] const QString& getName() const;
  void setName(const QString& name);

  [[nodiscard]] const FeatureTypes& getType() const;
  void setType(const FeatureTypes& type);

  void setCurrentNominal(const int& id, const QString& name,
                         const FeatureTypes& type);

  [[nodiscard]] const std::map<GeometryParameters, QString>&
  getUnknownNominalParameters() const;
  void setUnknownNominalParameters(
      const std::map<GeometryParameters, QString>& parameters);

 signals:

  // #######################
  // edit nominal parameters
  // #######################

  void nominalParametersChanged(
      const int& id, const std::map<GeometryParameters, double>& parameters);

 private slots:

  // #########################
  // actions triggered by user
  // #########################

  void on_toolButton_cancel_clicked();
  void on_toolButton_ok_clicked();

 private:
  // ##################################
  // methods to initialize GUI elements
  // ##################################

  void showEvent(QShowEvent* event) override;

  void initGUI();
  void initModels();

  // ##############
  // helper methods
  // ##############

  void setVisibility(const GeometryParameters& param, const bool& visible);

  void setParametersToGUI(const GeometryParameters& param,
                          const QString& value);
  void getParametersFromGUI(std::map<GeometryParameters, double>& parameters);

 private:
  Ui::NominalPropertiesDialog* ui;

  // ####################################################
  // attributes that describe the current nominal feature
  // ####################################################

  int id{-1};
  QString name;
  FeatureTypes type{FeatureTypes::eUndefinedFeature};
  std::map<GeometryParameters, QString> parameters;
};

#endif  // OI_APP_UI_DIALOGS_NOMINAL_PROPERTIES_DIALOG_H_

#ifndef OI_APP_UI_DIALOGS_CREATE_FEATURE_DIALOG_H_
#define OI_APP_UI_DIALOGS_CREATE_FEATURE_DIALOG_H_

#include <QDialog>
#include <QShowEvent>
#include <vector>

#include "feature_attributes.h"
#include "feature_wrapper.h"
#include "gui_util.h"
#include "model_manager.h"
#include "settings.h"
#include "util.h"

#if defined(OI_MAIN_LIB)
#define OI_MAIN_EXPORT Q_DECL_EXPORT
#else
#define OI_MAIN_EXPORT Q_DECL_IMPORT
#endif

using namespace oi;

namespace Ui {
class CreateFeatureDialog;
}

/*!
 * \brief The CreateFeatureDialog class
 */
class OI_MAIN_EXPORT CreateFeatureDialog : public QDialog {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(CreateFeatureDialog)

 public:
  explicit CreateFeatureDialog(QWidget* parent = nullptr);
  ~CreateFeatureDialog() override;

  // ###########################
  // get or set the feature type
  // ###########################

  [[nodiscard]] const FeatureTypes& getFeatureType() const;
  void setFeatureType(const FeatureTypes& type);

  bool featureCreated(bool created);

  void setCurrentJob(const oi::Guarded<OiJob>& job);

 signals:

  // #######################################
  // create features specified by attributes
  // #######################################

  void addFeatures(const FeatureAttributes& attributes);
  void initialized();  // currently used for testing

 private slots:

  // #########################
  // actions triggered by user
  // #########################

  void on_toolButton_ok_clicked();
  void on_toolButton_cancel_clicked();

  void on_comboBox_entityType_currentIndexChanged(int index);

  void on_comboBox_function_currentIndexChanged(const int index);

 private:
  // ##################################
  // methods to initialize GUI elements
  // ##################################

  void showEvent(QShowEvent* event) override;

  void initGUI();
  void initModels();

  void initFunctionsModel();
  void initMeasurementConfigModel();

  // ################
  // GUI interactions
  // ################

  void featureAttributesFromGUI(FeatureAttributes& attributes);

  void initMeasurementConfigUI(const int functionIndex);

  // D17: there is no isNominal/isActual checkbox any more - a nominal
  // producer is just a function whose shape says so (a ConstructFunction
  // declaring no needed elements, taking scalar parameters only). This
  // is what decides whether the nominal-system picker is relevant.
  [[nodiscard]] bool isNominalShapedFunction(
      const sys::Function& function) const;

 private:
  Ui::CreateFeatureDialog* ui;

  // #################
  // helper attributes
  // #################

  FeatureTypes typeOfFeature;
  std::vector<ElementTypes> neededElements;

  QPointer<AvailableFunctionsListProxyModel> functionListModel;
  QPointer<MeasurementConfigurationProxyModel> measurementConfigurationModel;

  void setDialogName();

  oi::Guarded<OiJob> currentJob;

  bool created{};
};

#endif  // OI_APP_UI_DIALOGS_CREATE_FEATURE_DIALOG_H_

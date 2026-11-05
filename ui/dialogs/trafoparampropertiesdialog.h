#ifndef TRAFOPARAMPROPERTIESDIALOG_H
#define TRAFOPARAMPROPERTIESDIALOG_H

#include <QDialog>
#include <QMap>
#include <QMessageBox>
#include <QPointer>
#include <QShowEvent>

#include "modelmanager.h"

using namespace oi;

namespace Ui {
class TrafoParamPropertiesDialog;
}

/*!
 * \brief The TrafoParamPropertiesDialog class
 */
class TrafoParamPropertiesDialog : public QDialog {
  Q_OBJECT

 public:
  explicit TrafoParamPropertiesDialog(QWidget* parent = nullptr);
  ~TrafoParamPropertiesDialog() override;

  // ############################################################
  // get or set information about the current trafo param feature
  // ############################################################

  [[nodiscard]] const int& getId() const;
  void setId(const int& id);

  [[nodiscard]] const QString& getName() const;
  void setName(const QString& name);

  void setCurrentTrafoParam(const int& id, const QString& name);

  [[nodiscard]] const QMap<TrafoParamParameters, QString>&
  getUnknownTrafoParamParameters() const;
  void setUnknownTrafoParamParameters(
      const QMap<TrafoParamParameters, QString>& parameters);

 signals:

  // ###########################
  // edit trafo param parameters
  // ###########################

  void trafoParamParametersChanged(
      const int& id, const QMap<TrafoParamParameters, double>& parameters);

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

  void setParametersToGUI(const TrafoParamParameters& param,
                          const QString& value);
  void getParametersFromGUI(QMap<TrafoParamParameters, double>& parameters);

 private:
  Ui::TrafoParamPropertiesDialog* ui;

  // ################################################
  // attributes that describe the current trafo param
  // ################################################

  int id{};
  QString name;
  QMap<TrafoParamParameters, QString> parameters;
};

#endif  // TRAFOPARAMPROPERTIESDIALOG_H

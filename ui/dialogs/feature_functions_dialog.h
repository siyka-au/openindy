#ifndef FEATUREFUNCTIONSDIALOG_H
#define FEATUREFUNCTIONSDIALOG_H

#include <QCloseEvent>
#include <QCompleter>
#include <QDebug>
#include <QDialog>
#include <QGridLayout>
#include <QMenu>
#include <QShowEvent>
#include <QStandardItemModel>
#include <QStringList>

#include "function_weight_delegate.h"
#include "model_manager.h"
#include "multi_selection_model.h"
#include "oi_job.h"
#include "scalar_parameter_widget.h"

using namespace oi;

// #include "customparameterwidget.h"
// #include "oimultiselectionmodel.h"

enum class Row { First = 0, Last };

namespace Ui {

class FeatureFunctionsDialog;
}

/*!
 * \brief The FeatureFunctionsDialog class
 */
class FeatureFunctionsDialog : public QDialog {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(FeatureFunctionsDialog)

 public:
  explicit FeatureFunctionsDialog(QWidget* parent = nullptr);
  ~FeatureFunctionsDialog() override;

 signals:

  // ##############################################################
  // recalculate the active feature after its functions were edited
  // ##############################################################

  void recalculateActiveFeature();

  void setFunctionPos(int pos);

 private slots:

  // #################################
  // scalar parameter changed callback
  // #################################

  void scalarParameterChanged();

  // #########################
  // actions triggered by user
  // #########################

  // functions table view
  void on_tableView_functionPlugins_clicked(const QModelIndex& index);
  void on_tableView_functionPlugins_doubleClicked(const QModelIndex& index);

  // active feature functions tree view
  void on_treeView_functions_clicked(const QModelIndex& index);

  // avaialable elements tree view
  void on_treeView_availableElements_clicked(const QModelIndex& index);
  void on_treeView_availableElements_entered(const QModelIndex& index);
  void on_treeView_availableElements_doubleClicked(const QModelIndex& index);

  // used elements tree view
  void on_treeView_usedElements_clicked(const QModelIndex& index);
  void on_treeView_usedElements_entered(const QModelIndex& index);
  void on_treeView_usedElements_doubleClicked(const QModelIndex& index);

  // add or remove elements
  void on_cmd_addElement_clicked();
  void on_cmd_removeElement_clicked();

  // active feature functions tree view context menu
  void activeFeatureFunctionsContextMenuRequested(const QPoint& point);
  void removeSelectedFunction();

  // recalculate the active feature
  void on_cmd_ok_clicked();

  // resize table views
  void resizeTableView();

  // check if the function supports weights
  bool checkSupportsWeights();

  // element filter selected
  void on_comboBox_features_currentIndexChanged(int index);
  void on_comboBox_groups_currentIndexChanged(int index);
  void on_lineEdit_searchByFeatureName_textChanged(const QString& arg1);

 private:
  Ui::FeatureFunctionsDialog* ui;

  // #################
  // helper attributes
  // #################

  // widget with scalar input parameters
  ScalarParameterWidget* scalarParameterWidget{};

  // ##################################
  // methods to initialize GUI elements
  // ##################################

  void showEvent(QShowEvent* event) override;

  void initGUI();
  void initModels();

  void setFunctionParameters(const QModelIndex& index);
  void setFunctionElements(const QModelIndex& index);

  void selectFunctionItem(Row row);
};

#endif  // FEATUREFUNCTIONSDIALOG_H

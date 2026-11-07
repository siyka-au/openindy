#ifndef WATCHWINDOW_H
#define WATCHWINDOW_H

#include <QCheckBox>
#include <QDialog>
#include <QKeyEvent>
#include <QLCDNumber>
#include <QLabel>
#include <QLineEdit>
#include <QMap>
#include <QPointer>
#include <QVBoxLayout>
#include <QVariantMap>
#include <atomic>
#include <functional>

#include "feature.h"
#include "featurewrapper.h"
#include "modelmanager.h"
#include "oijob.h"
#include "parameterdisplayconfig.h"
#include "station.h"
#include "trafocontroller.h"
#include "types.h"
#include "util.h"
#include "watchwindowutil.h"

enum DisplayAttributes {
  eName = 0,
  eX,
  eY,
  eZ,
  eD3D,
  eNotDeclared  // should be the last element!
};

enum class WatchWindowBehavior {
  eShowAlwaysActiveFeature = 0,  // show always the active feature
  eShowCurrentSelectedFeature,   // open new watch window for current selected
                                 // feature
  eShowNearestNominal
};

using namespace oi;

/*!
 * \brief The WatchWindowSettings class
 */
class WatchWindowSettings {
 public:
  WatchWindowSettings() {}

  // decimal digits for watch window values
  int digits{2};

  // reading type for the watch window values
  ReadingTypes readingType{ReadingTypes::eCartesianReading};

  // display values and tolerance
  QMap<DisplayAttributes, double> displayValues;

  bool showLastMeasurement{true};
};

namespace Ui {
class WatchWindowDialog;
}

/*!
 * \brief The WatchWindowDialog class
 * Handles displaying of differences between active feature (sensor) and its
 * nominal value.
 */
class WatchWindowDialog : public QDialog {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(WatchWindowDialog)

 public:
  explicit WatchWindowDialog(WatchWindowBehavior behavior,
                             oi::Guarded<OiJob> job,
                             QList<oi::Guarded<FeatureWrapper> > features,
                             QWidget* parent = nullptr);
  ~WatchWindowDialog() override;

 signals:

  // ######################################
  // inform about actions triggered by user
  // ######################################

  // ####################
  // start sensor actions
  // ####################

  void startMeasure();
  void useLastReading();
  void startStreaming(ReadingTypes streamFormat);
  void stopStreaming();

 private slots:

  // #########################
  // actions triggered by user
  // #########################

  // update settings
  void on_spinBox_decimalDigits_valueChanged(int arg1);
  void on_checkBox_x_clicked();
  void on_checkBox_y_clicked();
  void on_checkBox_z_clicked();
  void on_checkBox_d3d_clicked();
  void on_lineEdit_tolerance_x_textChanged(const QString& arg1);
  void on_lineEdit_tolerance_y_textChanged(const QString& arg1);
  void on_lineEdit_tolerance_z_textChanged(const QString& arg1);
  void on_lineEdit_tolerance_d3d_textChanged(const QString& arg1);

  // ##################################
  // update watch window display values
  // ##################################

  void realTimeReading(const QVariantMap& reading);

  // switch tab and update geometries
  void on_toolBox_currentChanged(int index);

  // hides x, y ,z, d3D if no current reading available
  void clearWatchWindow();

  void on_checkBox_showLastMeasurement_clicked();

  void keyPressEvent(QKeyEvent* e) override;

 private:
  // ##################################
  // methods to initialize GUI elements
  // ##################################

  void showEvent(QShowEvent* event) override;
  void closeEvent(QCloseEvent* event) override;

  void initGUI();

  // ##############################
  // connect job and active station
  // ##############################

  void connectSensor();
  void connectJob();

  // #############################################
  // helper methods to display watch window values
  // #############################################

  void setUpCartesianWatchWindow(const QVariantMap& reading);

  void getDefaultSettings();
  void resizeWatchWindowValues();

  Ui::WatchWindowDialog* ui;

  // #############################
  // current job and active sensor
  // #############################
  WatchWindowBehavior behavior;
  oi::Guarded<OiJob> currentJob;
  QList<oi::Guarded<FeatureWrapper> > features;
  // save active station here, to be able to disconnect it
  oi::Guarded<Station> activeStation;

  // ####################
  // dynamic GUI elements
  // ####################

  // watch window values
  QVBoxLayout* masterLayout{};
  QMap<DisplayAttributes, QLabel*> streamData;
  QMap<DisplayAttributes, int> masterLayoutIndex;

  // ################
  // display settings
  // ################

  WatchWindowSettings settings;

  // #################
  // helper attributes
  // #################

  TrafoController trafoController;

  // ###############################################
  // helper attribute to rescale the labels and text
  // ###############################################

  bool lablesRescaled;
  int oldWindowHeight;
  int oldWindowWidth;

  // set to "true" if watch window was currently updateded
  std::atomic<bool> watchWindowUpdated;

  void addLabel(DisplayAttributes att, QFont f);
  void setDisplayValue(DisplayAttributes attr, QString name,
                       std::function<double()> v);
  QString getNameLabel(oi::Guarded<FeatureWrapper> feature);

  oi::Guarded<FeatureWrapper> getFeature(OiVec trackerXYZ);
  void setVisibility();

  WatchWindowUtil util;
};

#endif  // WATCHWINDOW_H

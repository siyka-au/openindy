#ifndef PLUGININFOWIDGET_H
#define PLUGININFOWIDGET_H

#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPixmap>
#include <QTextBrowser>
#include <QVBoxLayout>
#include <QWidget>

#include "pluginrepository.h"
#include "types.h"
#include "util.h"

using namespace oi;

/*!
 * \brief The PluginInfoWidget class
 * Widget to display information about plugins
 */
class PluginInfoWidget : public QWidget {
  Q_OBJECT

 public:
  explicit PluginInfoWidget(QWidget* parent = 0);

 public slots:

  // ###############################################
  // display methods for different plugin interfaces
  // ###############################################

  void displayFunction(const sys::Function& function);
  void displaySensor(const sys::Sensor& sensor);
  void displaySimulation(const sys::Simulation& simulation);
  void displayTool(const sys::Tool& tool);
  void displayExchange(const sys::Exchange& exchange);

  void reset();

 private:
  // #################
  // helper attributes
  // #################

  // layout for the widget
  QVBoxLayout* myLayout;

  // general
  QLabel* label_description;
  QTextBrowser* txt_description;

  // function specific GUI elements
  QLabel* label_applicableFor;
  QHBoxLayout* layout_applicableFor;
  QLabel* label_neededElements;
  QHBoxLayout* layout_neededElements;
  QMap<FeatureTypes, QLabel*> applicableFor;   // labels with icons of features
  QMap<ElementTypes, QLabel*> neededElements;  // labels with icons of elements

  void initGUI();
  void clearLayout(QLayout* layout);
};

#endif  // PLUGININFOWIDGET_H

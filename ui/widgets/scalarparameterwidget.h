#ifndef SCALARPARAMETERWIDGET_H
#define SCALARPARAMETERWIDGET_H

#include <QComboBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMap>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QString>
#include <QWidget>
#include <QtCore>
#include <map>

class ScalarParameterWidget : public QWidget {
  Q_OBJECT

 public:
  explicit ScalarParameterWidget(QWidget* parent = nullptr);

 signals:

  // #############################################
  // inform aboud changes of the scalar parameters
  // #############################################

  void scalarParametersChanged();

 public slots:

  // ######################
  // set display parameters
  // ######################

  // fill scalar display parameters
  void setIntParameter(const std::map<QString, int>& intParameter);
  void setDoubleParameter(const std::map<QString, double>& doubleParameter);
  void setStringParameter(
      const std::multimap<QString, QString>& stringParameter,
      const std::map<QString, QString>& defaultSelection);

  // get scalar parameter selection (user defined inputs)
  const std::map<QString, int>& getIntParameter();
  const std::map<QString, double>& getDoubleParameter();
  const std::map<QString, QString>& getStringParameter();

  // remove all scalar display parameters
  void clearAll();

 private:
  // #################
  // helper attributes
  // #################

  // user defined parameters
  std::map<QString, int> intParameter;
  std::map<QString, double> doubleParameter;
  QMap<QString, QStringList> stringParameter;
  std::map<QString, QString> stringParameterSelection;

  // dynamic GUI elements for each available parameter
  QList<QLabel*> parameterLabel;
  QMap<QString, QLineEdit*> intParameterLineEdit;
  QMap<QString, QLineEdit*> doubleParameterLineEdit;
  QMap<QString, QComboBox*> stringParameterComboBox;

  // layout
  QFormLayout* myLayout;

  // regular expressions to validate user input
  QRegularExpression validInt;
  QRegularExpression validDouble;
  QRegularExpressionValidator* intValidator;
  QRegularExpressionValidator* doubleValidator;

  // ##############
  // helper methods
  // ##############

  void setUpGui();
  void clearGuiElements();
  void getParameterFromGUI();
};

#endif  // SCALARPARAMETERWIDGET_H

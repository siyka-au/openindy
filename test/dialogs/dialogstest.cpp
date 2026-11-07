#include <QList>
#include <QPointer>
#include <QString>
#include <QStringList>
#include <QStringListModel>
#include <QtTest>

#include "availablefunctionslistproxymodel.h"
#include "chooselalib.h"
#include "config_codec.h"
#include "controller.h"
#include "createfeaturedialog.h"
#include "measurementconfigurationdialog.h"
#include "projectexchanger.h"

// recomended readings:
// https://gist.github.com/peteristhegreat/cbd8eaa0e565d0b82dbfb5c7fdc61c8d
// https://vicrucann.github.io/tutorials/qttest-signals-qtreewidget/

using namespace oi;

class DialogsTest : public QObject {
  Q_OBJECT

 public:
  DialogsTest();

 private Q_SLOTS:
  void initTestCase();

  // functions my change global list of MeasurementConfigs
  // !!! keep order !!!
  void measurementConfigurationModel();

  void createPoint();
  void createCircle();
  void createPlane();
  void createPlaneFromPoints();
  void createLevel();
  void reuseDialogInstance();

  void measurementConfigJson_RW();
  void measurementConfigFilter();

  void measurementConfigDialog();

  void projectSaveLoad();

  void setDefaultFunctionAndConfig();
  void entityTypeComboResolvesByDataNotByVisibleText();

 private:
  void printMessage(const QString& msg, const MessageTypes& msgType,
                    const MessageDestinations& msgDest =
                        MessageDestinations::eConsoleMessage);
  QStringList getNames(QSortFilterProxyModel* model);
  QStringList getNames(QAbstractListModel* model);
  QStringList getNames(QAbstractItemModel* model);
  QStringList getNames(QList<MeasurementConfig> list);
  QStringList getKeys(QList<MeasurementConfig> list);
};

DialogsTest::DialogsTest() {}

void DialogsTest::printMessage(const QString& msg, const MessageTypes& msgType,
                               const MessageDestinations& msgDest) {
  qDebug() << msg;
}

void DialogsTest::initTestCase() {
  // the math library has no default backend; without this every OiMat
  // operation dereferences a null linear-algebra pointer (main.cpp does
  // the same at startup)
  ChooseLALib::setLinearAlgebra(ChooseLALib::LinearAlgebraLibrary::Armadillo);

  // the enum values themselves, not their names - the combo carries the type
  // in Qt::UserRole so its visible text is a display name and can be reworded
  QList<FeatureTypes> entityTypes;
  entityTypes << FeatureTypes::ePointFeature << FeatureTypes::eCircleFeature
              << FeatureTypes::ePlaneFeature;  // comboBox_entityType order

  QPointer<MeasurementConfigManager> measurementConfigManager =
      new MeasurementConfigManager();

  QObject::connect(measurementConfigManager.data(),
                   &MeasurementConfigManager::sendMessage, this,
                   &DialogsTest::printMessage);
  /*    QObject::connect(&ModelManager::getMeasurementConfigurationModel(),
     &MeasurementConfigurationModel::sendMessage, this,
     &DialogsTest::printMessage);
      QObject::connect(&ModelManager::getMeasurementConfigurationProxyModel(),
     &MeasurementConfigurationProxyModel::sendMessage, this,
     &DialogsTest::printMessage);*/

  // user config
  MeasurementConfig scanDistanceConfig;
  scanDistanceConfig.setName("measconfig-scandistance");  // not for point
  scanDistanceConfig.setMeasurementType(
      MeasurementTypes::eScanDistanceDependent_MeasurementType);
  scanDistanceConfig.setDistanceInterval(456);
  scanDistanceConfig.setMaxObservations(654);
  scanDistanceConfig.makeUserConfig();
  measurementConfigManager->saveUserConfig(scanDistanceConfig);

  MeasurementConfig fastPointConfig;
  fastPointConfig.setName("FastPoint");
  fastPointConfig.setMeasurementMode(MeasurementModes::eFast_MeasurementMode);
  fastPointConfig.setMeasurementType(
      MeasurementTypes::eSinglePoint_MeasurementType);
  fastPointConfig.makeUserConfig();
  measurementConfigManager->saveUserConfig(fastPointConfig);

  // project config
  MeasurementConfig fastPointConfigProject2;
  fastPointConfigProject2.setName("measconfig-fastpoint_project");
  fastPointConfigProject2.setMeasurementMode(
      MeasurementModes::eFast_MeasurementMode);
  fastPointConfigProject2.setMeasurementType(
      MeasurementTypes::eSinglePoint_MeasurementType);
  fastPointConfigProject2.makeProjectConfig();
  measurementConfigManager->addProjectConfig(fastPointConfigProject2);

  // create plugin with some functions, in the PluginRepository domain-object
  // shape, fed straight into ModelManager::testInit()'s injection seam -
  // no scan, no registration step needed
  sys::Plugin testPluginRef("testplugin", QString(), QString(), "esc",
                            QString(), QList<sys::Function>(),
                            QList<sys::Sensor>(), QList<sys::Tool>(),
                            QList<sys::Exchange>(), QList<sys::Simulation>());

  sys::Function sysPoint("function-fitpoint", QString(), testPluginRef,
                         QList<FeatureTypes>() << FeatureTypes::ePointFeature,
                         QList<ElementTypes>()
                             << ElementTypes::eObservationElement,
                         OiMetaData::iid_FitFunction);
  sys::Function sysPlane("function-fitplane", QString(), testPluginRef,
                         QList<FeatureTypes>() << FeatureTypes::ePlaneFeature,
                         QList<ElementTypes>()
                             << ElementTypes::eObservationElement,
                         OiMetaData::iid_FitFunction);
  sys::Function sysLevel(
      "function-fitlevel", QString(), testPluginRef,
      QList<FeatureTypes>()
          << FeatureTypes::ePlaneFeature << FeatureTypes::eLevelFeature,
      QList<ElementTypes>() << ElementTypes::eObservationElement,
      OiMetaData::iid_FitFunction);
  sys::Function sysPlaneFromPoints(
      "function-planefrompoints", QString(), testPluginRef,
      QList<FeatureTypes>() << FeatureTypes::ePlaneFeature,
      QList<ElementTypes>() << ElementTypes::eObservationElement,
      OiMetaData::iid_ConstructFunction);  // set measurement config combox to
                                           // visible = false
  sys::Function sysCircle("function-fitcircle", QString(), testPluginRef,
                          QList<FeatureTypes>() << FeatureTypes::eCircleFeature,
                          QList<ElementTypes>()
                              << ElementTypes::eObservationElement,
                          OiMetaData::iid_FitFunction);

  QList<sys::Function> sysFunctions;
  sysFunctions << sysPlane << sysPoint << sysLevel << sysCircle
               << sysPlaneFromPoints;

  sys::Plugin sysTestPlugin("testplugin", QString(), QString(), "esc",
                            QString(), sysFunctions, QList<sys::Sensor>(),
                            QList<sys::Tool>(), QList<sys::Exchange>(),
                            QList<sys::Simulation>());

  QList<sys::Plugin> sysPlugins;
  sysPlugins << sysTestPlugin;

  // init ModelManager
  ModelManager::testInit(entityTypes, measurementConfigManager, sysPlugins);
}

void DialogsTest::createPoint() {
  // create dialog
  CreateFeatureDialog dialog;

  // comboBox_entityType currently not usesd, type is set directly
  dialog.setFeatureType(FeatureTypes::ePointFeature);
  dialog.show();  // to call: void showEvent(QShowEvent *event); and initialize
                  // dialog
  QSignalSpy spy_initialized(&dialog, SIGNAL(initialized()));
  spy_initialized.wait(1000);

  // check function
  QPointer<QComboBox> functionCB;
  QPointer<QListView> functionLV;

  // check for default function
  functionCB = dialog.findChild<QComboBox*>("comboBox_function");
  functionLV = functionCB->findChild<QListView*>();
  qDebug() << "rowCount" << functionLV->model()->rowCount();
  qDebug() << functionLV->model()->index(0, 0).data(Qt::DisplayRole).toString();
  qDebug() << "currentIndex" << functionCB->currentIndex();
  QVERIFY("function-fitpoint" ==
          functionLV->model()->index(0, 0).data(Qt::DisplayRole).toString());

  // check applicable measurement configs
  QPointer<QComboBox> mConfigCB =
      dialog.findChild<QComboBox*>("comboBox_mConfig");
  QPointer<QListView> mConfigLV = mConfigCB->findChild<QListView*>();
  qDebug() << getNames(mConfigLV->model());
  // PseudoTracker and LeicaTachymeter each publish their own FastPoint/
  // PrecisePoint/StdPoint/StdTwoSide now (sensor-type-tagged, no longer one
  // shared hardcoded list) - with no active station in this test, the type
  // filter can't narrow to one sensor, so both show up. Which of a pair of
  // identically-named entries sorts first isn't meaningful, so check the
  // full multiset rather than assert brittle per-index identity.
  QVERIFY(10 == mConfigLV->model()->rowCount());
  QCOMPARE(
      getNames(mConfigLV->model()),
      QStringList({"FastPoint", "FastPoint", "measconfig-fastpoint_project",
                   "measconfig-scandistance [user]", "PrecisePoint",
                   "PrecisePoint", "StdPoint", "StdPoint", "StdTwoSide",
                   "StdTwoSide"}));
}

void DialogsTest::createCircle() {
  // create dialog
  CreateFeatureDialog dialog;

  // comboBox_entityType currently not usesd, type is set directly
  dialog.setFeatureType(FeatureTypes::eCircleFeature);
  dialog.show();  // to call: void showEvent(QShowEvent *event); and initialize
                  // dialog
  QSignalSpy spy_initialized(&dialog, SIGNAL(initialized()));
  spy_initialized.wait(1000);

  // check function
  QPointer<QComboBox> functionCB;
  QPointer<QListView> functionLV;

  // check for default function
  functionCB = dialog.findChild<QComboBox*>("comboBox_function");
  functionLV = functionCB->findChild<QListView*>();
  qDebug() << "rowCount" << functionLV->model()->rowCount();
  qDebug() << functionLV->model()->index(0, 0).data(Qt::DisplayRole).toString();
  qDebug() << "currentIndex" << functionCB->currentIndex();
  QVERIFY("function-fitcircle" ==
          functionLV->model()->index(0, 0).data(Qt::DisplayRole).toString());

  // check applicable measurement configs
  QPointer<QComboBox> mConfigCB =
      dialog.findChild<QComboBox*>("comboBox_mConfig");
  QPointer<QListView> mConfigLV = mConfigCB->findChild<QListView*>();
  qDebug() << getNames(mConfigLV->model());
  // PseudoTracker and LeicaTachymeter each publish their own FastPoint/
  // PrecisePoint/StdPoint/StdTwoSide now (sensor-type-tagged, no longer one
  // shared hardcoded list) - with no active station in this test, the type
  // filter can't narrow to one sensor, so both show up. Which of a pair of
  // identically-named entries sorts first isn't meaningful, so check the
  // full multiset rather than assert brittle per-index identity.
  QVERIFY(10 == mConfigLV->model()->rowCount());
  QCOMPARE(
      getNames(mConfigLV->model()),
      QStringList({"FastPoint", "FastPoint", "measconfig-fastpoint_project",
                   "measconfig-scandistance [user]", "PrecisePoint",
                   "PrecisePoint", "StdPoint", "StdPoint", "StdTwoSide",
                   "StdTwoSide"}));
}

void DialogsTest::createPlane() {
  // create dialog
  CreateFeatureDialog dialog;

  // comboBox_entityType currently not usesd, type is set directly
  dialog.setFeatureType(FeatureTypes::ePlaneFeature);
  dialog.show();  // to call: void showEvent(QShowEvent *event); and initialize
                  // dialog
  QSignalSpy spy_initialized(&dialog, SIGNAL(initialized()));
  spy_initialized.wait(1000);

  // check function
  QPointer<QComboBox> functionCB;
  QPointer<QListView> functionLV;

  // check for default function
  functionCB = dialog.findChild<QComboBox*>("comboBox_function");
  functionLV = functionCB->findChild<QListView*>();
  qDebug() << "rowCount" << functionLV->model()->rowCount();
  qDebug() << functionLV->model()->index(0, 0).data(Qt::DisplayRole).toString();
  qDebug() << "currentIndex" << functionCB->currentIndex();
  QVERIFY("function-fitplane" == functionLV->model()
                                     ->index(functionCB->currentIndex(), 0)
                                     .data(Qt::DisplayRole)
                                     .toString());

  // check applicable measurement configs
  QPointer<QComboBox> mConfigCB =
      dialog.findChild<QComboBox*>("comboBox_mConfig");
  QPointer<QListView> mConfigLV = mConfigCB->findChild<QListView*>();
  qDebug() << getNames(mConfigLV->model());
  // PseudoTracker and LeicaTachymeter each publish their own FastPoint/
  // PrecisePoint/StdPoint/StdTwoSide now (sensor-type-tagged, no longer one
  // shared hardcoded list) - with no active station in this test, the type
  // filter can't narrow to one sensor, so both show up. Which of a pair of
  // identically-named entries sorts first isn't meaningful, so check the
  // full multiset rather than assert brittle per-index identity.
  QVERIFY(10 == mConfigLV->model()->rowCount());
  QCOMPARE(
      getNames(mConfigLV->model()),
      QStringList({"FastPoint", "FastPoint", "measconfig-fastpoint_project",
                   "measconfig-scandistance [user]", "PrecisePoint",
                   "PrecisePoint", "StdPoint", "StdPoint", "StdTwoSide",
                   "StdTwoSide"}));
}

void DialogsTest::createPlaneFromPoints() {
  // create dialog
  CreateFeatureDialog dialog;

  // comboBox_entityType currently not usesd, type is set directly
  dialog.setFeatureType(FeatureTypes::ePlaneFeature);
  dialog.show();  // to call: void showEvent(QShowEvent *event); and initialize
                  // dialog
  QSignalSpy spy_initialized(&dialog, SIGNAL(initialized()));
  spy_initialized.wait(1000);

  // check function
  QPointer<QComboBox> functionCB;
  QPointer<QListView> functionLV;

  // check for default function
  functionCB = dialog.findChild<QComboBox*>("comboBox_function");
  functionLV = functionCB->findChild<QListView*>();
  qDebug() << "rowCount" << functionLV->model()->rowCount();
  qDebug() << functionLV->model()->index(0, 0).data(Qt::DisplayRole).toString();
  qDebug() << "currentIndex" << functionCB->currentIndex();
  QVERIFY("function-fitplane" == functionLV->model()
                                     ->index(functionCB->currentIndex(), 0)
                                     .data(Qt::DisplayRole)
                                     .toString());

  int i = 0;
  for (i = 0; i < functionLV->model()->rowCount(); i++) {
    if ("function-planefrompoints" ==
        functionLV->model()->index(i, 0).data(Qt::DisplayRole).toString()) {
      break;
    }
  }

  // select "function-planefrompoints"
  QTest::mouseClick(functionCB, Qt::LeftButton);
  QTest::qWait(1000);  // TODO spy

  QModelIndex idx = functionLV->model()->index(i, 0);
  functionLV->scrollTo(idx);

  QPoint itemPt = functionLV->visualRect(idx).center();
  QString functionName =
      functionLV->model()->index(i, 0).data(Qt::DisplayRole).toString();
  qDebug() << "clicking on function" << functionName;
  QVERIFY("function-planefrompoints" == functionName);

  QTest::mouseClick(functionLV->viewport(), Qt::LeftButton, Qt::NoModifier,
                    itemPt);
  QTest::qWait(1000);

  // check applicable measurement configs
  QPointer<QComboBox> mConfigCB =
      dialog.findChild<QComboBox*>("comboBox_mConfig");
  QPointer<QListView> mConfigLV = mConfigCB->findChild<QListView*>();
  qDebug() << getNames(mConfigLV->model());

  QVERIFY(0 == mConfigLV->model()->rowCount());
}

void DialogsTest::createLevel() {
  // create dialog
  CreateFeatureDialog dialog;

  // comboBox_entityType currently not usesd, type is set directly
  dialog.setFeatureType(FeatureTypes::ePlaneFeature);
  dialog.show();  // to call: void showEvent(QShowEvent *event); and initialize
                  // dialog
  QSignalSpy spy_initialized(&dialog, SIGNAL(initialized()));
  spy_initialized.wait(1000);

  // check function
  QPointer<QComboBox> functionCB;
  QPointer<QListView> functionLV;

  // check for default function
  functionCB = dialog.findChild<QComboBox*>("comboBox_function");
  functionLV = functionCB->findChild<QListView*>();
  qDebug() << "rowCount" << functionLV->model()->rowCount();
  qDebug() << functionLV->model()->index(0, 0).data(Qt::DisplayRole).toString();
  qDebug() << "currentIndex" << functionCB->currentIndex();
  QVERIFY(3 == functionLV->model()->rowCount());
  QVERIFY("function-fitplane" == functionLV->model()
                                     ->index(functionCB->currentIndex(), 0)
                                     .data(Qt::DisplayRole)
                                     .toString());

  int i = 0;
  for (i = 0; i < functionLV->model()->rowCount(); i++) {
    if ("function-fitlevel" ==
        functionLV->model()->index(i, 0).data(Qt::DisplayRole).toString()) {
      break;
    }
  }

  // select "function-fitlevel"
  QTest::mouseClick(functionCB, Qt::LeftButton);
  QTest::qWait(1000);  // TODO spy

  QModelIndex idx = functionLV->model()->index(i, 0);
  functionLV->scrollTo(idx);

  QPoint itemPt = functionLV->visualRect(idx).center();
  QString functionName =
      functionLV->model()->index(i, 0).data(Qt::DisplayRole).toString();
  qDebug() << "clicking on function" << functionName;
  QVERIFY("function-fitlevel" == functionName);

  QTest::mouseClick(functionLV->viewport(), Qt::LeftButton, Qt::NoModifier,
                    itemPt);
  QTest::qWait(1000);

  // check applicable measurement configs
  QPointer<QComboBox> mConfigCB =
      dialog.findChild<QComboBox*>("comboBox_mConfig");
  QPointer<QListView> mConfigLV = mConfigCB->findChild<QListView*>();
  qDebug() << getNames(mConfigLV->model());
  QVERIFY(1 == mConfigLV->model()->rowCount());
  QVERIFY("level" ==
          mConfigLV->model()->index(0, 0).data(Qt::DisplayRole).toString());
}

void DialogsTest::reuseDialogInstance() {
  // create dialog
  CreateFeatureDialog dialog;

  // comboBox_entityType currently not usesd, type is set directly
  dialog.setFeatureType(FeatureTypes::ePlaneFeature);
  dialog.show();  // to call: void showEvent(QShowEvent *event); and initialize
                  // dialog
  dialog.activateWindow();
  QSignalSpy spy_initialized(&dialog, SIGNAL(initialized()));
  spy_initialized.wait(1000);

  // check function
  QPointer<QComboBox> functionCB;
  QPointer<QListView> functionLV;

  // check for default function
  functionCB = dialog.findChild<QComboBox*>("comboBox_function");
  functionLV = functionCB->findChild<QListView*>();
  qDebug() << "rowCount" << functionLV->model()->rowCount();
  qDebug() << functionLV->model()->index(0, 0).data(Qt::DisplayRole).toString();
  qDebug() << "currentIndex" << functionCB->currentIndex();
  QVERIFY("function-fitplane" == functionLV->model()
                                     ->index(functionCB->currentIndex(), 0)
                                     .data(Qt::DisplayRole)
                                     .toString());

  // check applicable measurement configs
  QPointer<QComboBox> mConfigCB =
      dialog.findChild<QComboBox*>("comboBox_mConfig");
  QPointer<QListView> mConfigLV = mConfigCB->findChild<QListView*>();
  qDebug() << getNames(mConfigLV->model());
  // PseudoTracker and LeicaTachymeter each publish their own FastPoint/
  // PrecisePoint/StdPoint/StdTwoSide now (sensor-type-tagged, no longer one
  // shared hardcoded list) - with no active station in this test, the type
  // filter can't narrow to one sensor, so both show up. Which of a pair of
  // identically-named entries sorts first isn't meaningful, so check the
  // full multiset rather than assert brittle per-index identity.
  QVERIFY(10 == mConfigLV->model()->rowCount());
  QCOMPARE(
      getNames(mConfigLV->model()),
      QStringList({"FastPoint", "FastPoint", "measconfig-fastpoint_project",
                   "measconfig-scandistance [user]", "PrecisePoint",
                   "PrecisePoint", "StdPoint", "StdPoint", "StdTwoSide",
                   "StdTwoSide"}));

  dialog.close();
  QTest::qWait(1000);
  dialog.show();
  dialog.activateWindow();

  // check for default function
  functionCB = dialog.findChild<QComboBox*>("comboBox_function");
  functionLV = functionCB->findChild<QListView*>();
  qDebug() << "rowCount" << functionLV->model()->rowCount();
  qDebug() << functionLV->model()->index(0, 0).data(Qt::DisplayRole).toString();
  qDebug() << "currentIndex" << functionCB->currentIndex();
  QVERIFY(3 == functionLV->model()->rowCount());
  QVERIFY("function-fitplane" == functionLV->model()
                                     ->index(functionCB->currentIndex(), 0)
                                     .data(Qt::DisplayRole)
                                     .toString());

  int i = 0;
  for (i = 0; i < functionLV->model()->rowCount(); i++) {
    if ("function-fitlevel" ==
        functionLV->model()->index(i, 0).data(Qt::DisplayRole).toString()) {
      break;
    }
  }

  // select "function-fitlevel"
  QTest::mouseClick(functionCB, Qt::LeftButton);
  QTest::qWait(1000);  // TODO spy

  QModelIndex idx = functionLV->model()->index(i, 0);
  functionLV->scrollTo(idx);

  QPoint itemPt = functionLV->visualRect(idx).center();
  QString functionName =
      functionLV->model()->index(i, 0).data(Qt::DisplayRole).toString();
  qDebug() << "clicking on function" << functionName;
  QVERIFY("function-fitlevel" == functionName);

  QTest::mouseClick(functionLV->viewport(), Qt::LeftButton, Qt::NoModifier,
                    itemPt);
  QTest::qWait(1000);

  // check applicable measurement configs
  mConfigCB = dialog.findChild<QComboBox*>("comboBox_mConfig");
  mConfigLV = mConfigCB->findChild<QListView*>();
  qDebug() << getNames(mConfigLV->model());
  QVERIFY(1 == mConfigLV->model()->rowCount());
  QVERIFY("level" ==
          mConfigLV->model()->index(0, 0).data(Qt::DisplayRole).toString());
}

void DialogsTest::measurementConfigJson_RW() {
  MeasurementConfig config;
  config.setName("measconfig-scantime");
  config.setMeasurementMode(MeasurementModes::ePrecise_MeasurementMode);
  config.setMeasurementType(
      MeasurementTypes::eScanTimeDependent_MeasurementType);
  config.setMeasureTwoSides(true);
  config.setMaxObservations(17);
  config.setTimeInterval(1.5);
  config.setDistanceInterval(0.25);

  const auto decoded =
      codec::decodeMeasurementConfig(codec::encodeMeasurementConfig(config));
  QVERIFY(decoded.has_value());
  QCOMPARE(decoded->getName(), config.getName());
  const MeasurementConfig& fromJson = *decoded;

  QVERIFY(config.getMeasurementMode() == fromJson.getMeasurementMode());
  QVERIFY(config.getMeasurementType() == fromJson.getMeasurementType());
  QVERIFY(config.getMeasureTwoSides() == fromJson.getMeasureTwoSides());
  QVERIFY(config.getMaxObservations() == fromJson.getMaxObservations());
  QVERIFY(config.getTimeInterval() == fromJson.getTimeInterval());
  QVERIFY(config.getDistanceInterval() == fromJson.getDistanceInterval());
}

QStringList DialogsTest::getNames(QSortFilterProxyModel* model) {
  QStringList names;
  for (int row = 0; row < model->rowCount(); row++) {
    names << model->index(row, 0).data(Qt::DisplayRole).toString();
  }
  return names;
}

QStringList DialogsTest::getNames(QAbstractListModel* model) {
  QStringList names;
  for (int row = 0; row < model->rowCount(); row++) {
    names << model->index(row, 0).data(Qt::DisplayRole).toString();
  }
  return names;
}

QStringList DialogsTest::getNames(QAbstractItemModel* model) {
  QStringList names;
  for (int row = 0; row < model->rowCount(); row++) {
    names << model->index(row, 0).data(Qt::DisplayRole).toString();
  }
  return names;
}

QStringList DialogsTest::getNames(QList<MeasurementConfig> list) {
  QStringList names;
  foreach (const MeasurementConfig mc, list) {
    names << mc.getName();
  }
  return names;
}

QStringList DialogsTest::getKeys(QList<MeasurementConfig> list) {
  QStringList keys;
  foreach (const MeasurementConfig mc, list) {
    MeasurementConfigKey key = mc.getKey();
    QString t;
    switch (key.getConfigType()) {
      case ConfigTypes::eUserConfig:
        t = "user";
        break;
      case ConfigTypes::eProjectConfig:
        t = "project";
        break;
      case ConfigTypes::eStandardConfig:
        t = "standard";
        break;
    }

    keys << QString("(%1, %2)").arg(key.getName()).arg(t);
  }
  return keys;
}

void DialogsTest::measurementConfigFilter() {
  MeasurementConfigurationProxyModel* proxy =
      &ModelManager::getMeasurementConfigurationProxyModel();  // global test
                                                               // instance
  MeasurementConfigurationModel* sourceModel =
      static_cast<MeasurementConfigurationModel*>(proxy->sourceModel());

  QStringList names;

  names = getNames(sourceModel);
  qDebug() << "source model:   " << names;

  names = getNames(proxy);
  qDebug() << "current:        " << names;
  proxy->setFilter(true);
  names = getNames(proxy);
  qDebug() << "all:            " << names;

  QList<ElementTypes> neededElements;
  QList<FeatureTypes> applicableFor;
  neededElements.append(ElementTypes::eObservationElement);
  applicableFor.append(FeatureTypes::ePointFeature);
  proxy->setFilter(neededElements, FeatureTypes::ePlaneFeature, applicableFor);
  names = getNames(proxy);
  qDebug() << "point:          " << names;

  neededElements.clear();
  applicableFor.clear();
  neededElements.append(ElementTypes::eObservationElement);
  applicableFor.append(FeatureTypes::ePlaneFeature);
  proxy->setFilter(neededElements, FeatureTypes::ePlaneFeature, applicableFor);
  names = getNames(proxy);
  qDebug() << "plane:          " << names;

  applicableFor.clear();
  applicableFor.append(FeatureTypes::eLevelFeature);
  proxy->setFilter(neededElements, FeatureTypes::ePlaneFeature, applicableFor);
  names = getNames(proxy);
  qDebug() << "level:          " << names;

  proxy->setFilterProjectConfig();
  names = getNames(proxy);
  qDebug() << "project config: " << names;
  // FastPoint/PrecisePoint/StdPoint/StdTwoSide doubled: PseudoTracker and
  // LeicaTachymeter each publish their own now. "level" is tracker-only
  // (LeicaTachymeter doesn't support level readings), so it stays singular.
  QCOMPARE(names, QStringList({"FastPoint", "FastPoint", "level",
                               "measconfig-fastpoint_project", "PrecisePoint",
                               "PrecisePoint", "StdPoint", "StdPoint",
                               "StdTwoSide", "StdTwoSide"}));

  proxy->setFilterUserConfig();
  names = getNames(proxy);
  qDebug() << "user config:    " << names;
  QVERIFY(2 == proxy->rowCount());
  QVERIFY("FastPoint" == proxy->index(0, 0).data(Qt::DisplayRole).toString());
  QVERIFY("measconfig-scandistance" ==
          proxy->index(1, 0).data(Qt::DisplayRole).toString());

  proxy->setFilter(true);
  names = getNames(proxy);
  qDebug() << "all:            " << names;
  QCOMPARE(names, QStringList({"FastPoint", "FastPoint", "FastPoint", "level",
                               "measconfig-fastpoint_project",
                               "measconfig-scandistance", "PrecisePoint",
                               "PrecisePoint", "StdPoint", "StdPoint",
                               "StdTwoSide", "StdTwoSide"}));
}

void DialogsTest::measurementConfigDialog() {
  MeasurementConfig projectFastPointConfig =
      ModelManager::getMeasurementConfigManager()->getProjectConfig(
          "measconfig-fastpoint");

  // create dialog
  MeasurementConfigurationDialog dialog;

  // MeasurementConfig config =
  // measurementConfigManager->getProjectConfig(projectFastPointConfig.getName());
  dialog.setMeasurementConfiguration(projectFastPointConfig);
  dialog.show();  // to call: void showEvent(QShowEvent *event); and initialize
                  // dialog
  QSignalSpy spy_initialized(&dialog, SIGNAL(initialized()));
  spy_initialized.wait(1000);

  // add new config
  QPointer<QWidget> addPB = dialog.findChild<QWidget*>("pushButton_add");

  QTest::mouseClick(addPB, Qt::LeftButton);
  QTest::qWait(1000);  // TODO spy

  QPointer<QListView> userConfigsLV =
      dialog.findChild<QListView*>("listView_userConfigs");
  qDebug() << getNames(userConfigsLV->model());

  QVERIFY(3 == userConfigsLV->model()->rowCount());
  QVERIFY("FastPoint" ==
          userConfigsLV->model()->index(0, 0).data(Qt::DisplayRole).toString());
  QVERIFY("measconfig-scandistance" ==
          userConfigsLV->model()->index(1, 0).data(Qt::DisplayRole).toString());
  QVERIFY("new config" ==
          userConfigsLV->model()->index(2, 0).data(Qt::DisplayRole).toString());

  // select project config: "PrecisePoint"
  QPointer<QTabWidget> tabW = dialog.findChild<QTabWidget*>("tabWidget");
  tabW->setCurrentIndex(0);  // TODO change tab by mouseClick ???

  QPointer<QListView> projectConfigsLV =
      dialog.findChild<QListView*>("listView_projectConfigs");
  qDebug() << getNames(projectConfigsLV->model());

  // was asserting against userConfigsLV here (stale from before the tab
  // switch above) - a pre-existing copy/paste bug that happened not to
  // matter while the counts coincided; the assertions below are clearly
  // about the project config list, so check that one. FastPoint/PrecisePoint/
  // StdPoint/StdTwoSide doubled: PseudoTracker and LeicaTachymeter each
  // publish their own now; "level" stays singular (tracker-only).
  QCOMPARE(getNames(projectConfigsLV->model()),
           QStringList({"FastPoint", "FastPoint", "level",
                        "measconfig-fastpoint_project", "PrecisePoint",
                        "PrecisePoint", "StdPoint", "StdPoint", "StdTwoSide",
                        "StdTwoSide"}));

  // find index for "PrecisePoint"
  int i = 0;
  for (i = 0; i < projectConfigsLV->model()->rowCount(); i++) {
    if ("PrecisePoint" == projectConfigsLV->model()
                              ->index(i, 0)
                              .data(Qt::DisplayRole)
                              .toString()) {
      break;
    }
  }

  // right click "PrecisePoint"
  QModelIndex idx = projectConfigsLV->model()->index(i, 0);
  projectConfigsLV->scrollTo(idx);
  QPoint itemPt = projectConfigsLV->visualRect(idx).center();
  QTest::mouseClick(projectConfigsLV->viewport(), Qt::LeftButton,
                    Qt::NoModifier, itemPt);
  QTest::qWait(1000);
  QPointer<QLabel> configNameL = dialog.findChild<QLabel*>("label_configName");
  QVERIFY("PrecisePoint" == configNameL->text());

  // clean up
  ModelManager::getMeasurementConfigManager()->removeUserConfig("new config");
}

void DialogsTest::measurementConfigurationModel() {
  MeasurementConfigurationProxyModel* proxy =
      &ModelManager::getMeasurementConfigurationProxyModel();  // global test
                                                               // instance
  MeasurementConfigurationModel* sourceModel =
      static_cast<MeasurementConfigurationModel*>(proxy->sourceModel());

  proxy->setFilter(true);
  qDebug() << getNames(proxy).join(", ");
  // FastPoint/PrecisePoint/StdPoint/StdTwoSide each appear twice: PseudoTracker
  // and LeicaTachymeter now each publish their own (sensor-type-tagged)
  // standard catalog rather than sharing one hardcoded list - "level" is
  // tracker-only, since LeicaTachymeter doesn't support level readings at all.
  // Plus this test's own user "FastPoint" and its user/project fixtures.
  QVERIFY(
      "FastPoint, FastPoint, FastPoint, level, measconfig-fastpoint_project, "
      "measconfig-scandistance, PrecisePoint, PrecisePoint, StdPoint, "
      "StdPoint, StdTwoSide, StdTwoSide" == getNames(proxy).join(", "));

  proxy->setFilterUserConfig();

  qDebug() << getNames(proxy);
  QVERIFY("FastPoint, measconfig-scandistance" == getNames(proxy).join(", "));
  // rename
  QString value("new_name");
  QModelIndex index = proxy->index(0, 0);
  MeasurementConfig backup =
      sourceModel->getMeasurementConfig(proxy->mapToSource(index));
  proxy->setData(index, value, Qt::EditRole);

  proxy->setFilterUserConfig();
  qDebug() << getNames(proxy);
  qDebug() << getNames(proxy).join(", ");
  QVERIFY("measconfig-scandistance, new_name" == getNames(proxy).join(", "));

  // remove
  index = proxy->index(1, 0);
  sourceModel->removeMeasurementConfig(proxy->mapToSource(index));

  proxy->setFilterUserConfig();
  qDebug() << getNames(proxy).join(", ");
  QVERIFY("measconfig-scandistance" == getNames(proxy).join(", "));

  // clean up / previouse content
  ModelManager::getMeasurementConfigManager()->saveUserConfig(backup);
  // proxy->setFilterUserConfig();
  qDebug() << getNames(proxy).join(", ");
  QVERIFY("FastPoint, measconfig-scandistance" == getNames(proxy).join(", "));

  proxy->setFilter(true);
  qDebug() << getNames(proxy).join(", ");
  // FastPoint/PrecisePoint/StdPoint/StdTwoSide each appear twice: PseudoTracker
  // and LeicaTachymeter now each publish their own (sensor-type-tagged)
  // standard catalog rather than sharing one hardcoded list - "level" is
  // tracker-only, since LeicaTachymeter doesn't support level readings at all.
  // Plus this test's own user "FastPoint" and its user/project fixtures.
  QVERIFY(
      "FastPoint, FastPoint, FastPoint, level, measconfig-fastpoint_project, "
      "measconfig-scandistance, PrecisePoint, PrecisePoint, StdPoint, "
      "StdPoint, StdTwoSide, StdTwoSide" == getNames(proxy).join(", "));
}

void DialogsTest::projectSaveLoad() {
  OiJob jobS;

  QPointer<FeatureWrapper> stationFeature = new FeatureWrapper();
  QPointer<Station> station = new Station();
  station->setFeatureName("STATION01");
  stationFeature->setStation(station);
  jobS.addFeature(stationFeature);

  QPointer<FeatureWrapper> systemFeature = new FeatureWrapper();
  QPointer<CoordinateSystem> system = new CoordinateSystem();
  system->setFeatureName("PART");
  systemFeature->setCoordinateSystem(system);
  jobS.addFeature(systemFeature);

  // activate features
  station->setActiveStationState(true);
  station->getCoordinateSystem()->setActiveCoordinateSystemState(true);

  // create feature attributes
  FeatureAttributes attr;
  attr.count = 1;
  attr.typeOfFeature = FeatureTypes::eCoordinateSystemFeature;
  attr.name = "Bundle01";
  attr.isBundleSystem = true;

  // add feature
  jobS.addFeatures(attr);

  FeatureAttributes point1;
  point1.count = 1;
  point1.typeOfFeature = FeatureTypes::ePointFeature;
  point1.name = "Point01";
  point1.measurementConfig =
      ModelManager::getMeasurementConfigManager()->getProjectConfig(
          "measconfig-fastpoint_project");
  QVERIFY(point1.measurementConfig.isValid());
  point1.functionPlugin = std::pair<QString, QString>("function1", "path");
  jobS.addFeatures(point1);  // job is reponsible
  QPointer<FeatureWrapper> feature = jobS.getFeaturesByName(point1.name).at(0);
  // feature->getFeature()->addFunction(function); // controller ist responsible
  feature->getGeometry()->setMeasurementConfig(
      point1.measurementConfig.getKey());  // controller ist responsible

  ProjectExchanger projectExchanger;
  projectExchanger.setMeasurementConfigManager(
      ModelManager::getMeasurementConfigManager());

  const QByteArray project = projectExchanger.saveProject(&jobS);
  QVERIFY(!project.isEmpty());

  QString name =
      getKeys(ModelManager::getMeasurementConfigManager()->getConfigs())
          .join(", ");
  qDebug() << "a" << name;
  // project, standard & user configs - FastPoint/PrecisePoint/StdPoint/
  // StdTwoSide each appear as two "standard" entries now (one per sensor
  // type; getKeys() doesn't render sensor type, so they're indistinguishable
  // in this string, just doubled)
  QVERIFY(
      "(FastPoint, user), (FastPoint, standard), (FastPoint, standard), "
      "(PrecisePoint, standard), (PrecisePoint, standard), (StdPoint, "
      "standard), (StdPoint, standard), (StdTwoSide, standard), (StdTwoSide, "
      "standard), (level, standard), (measconfig-fastpoint_project, project), "
      "(measconfig-scandistance, user)" == name);

  // clean up
  foreach (const MeasurementConfig mc,
           ModelManager::getMeasurementConfigManager()->getConfigs()) {
    if (mc.isProjectConfig() && !mc.isStandardConfig()) {
      ModelManager::getMeasurementConfigManager()->removeProjectConfig(
          mc.getName());
    }
  }

  name = getKeys(ModelManager::getMeasurementConfigManager()->getConfigs())
             .join(", ");
  qDebug() << "b" << name;
  // standard & user configs
  QVERIFY(
      "(FastPoint, user), (FastPoint, standard), (FastPoint, standard), "
      "(PrecisePoint, standard), (PrecisePoint, standard), (StdPoint, "
      "standard), (StdPoint, standard), (StdTwoSide, standard), (StdTwoSide, "
      "standard), (level, standard), (measconfig-scandistance, user)" == name);

  QPointer<OiJob> jobL = projectExchanger.loadProject(project).job;

  name = getKeys(ModelManager::getMeasurementConfigManager()->getConfigs())
             .join(", ");
  qDebug() << "c" << name;
  // project, standard & user configs
  QVERIFY(
      "(FastPoint, user), (FastPoint, standard), (FastPoint, standard), "
      "(PrecisePoint, standard), (PrecisePoint, standard), (StdPoint, "
      "standard), (StdPoint, standard), (StdTwoSide, standard), (StdTwoSide, "
      "standard), (level, standard), (measconfig-fastpoint_project, project), "
      "(measconfig-scandistance, user)" == name);
}

void DialogsTest::setDefaultFunctionAndConfig() {
  QPointer<AvailableFunctionsListProxyModel> proxyModel =
      ModelManager::getAvailableFunctionsProxyModel();
  AvailableFunctionsListModel* sourceModel =
      dynamic_cast<AvailableFunctionsListModel*>(proxyModel->sourceModel());
  QVERIFY(sourceModel != nullptr);

  foreach (const sys::Plugin& plugin, sourceModel->getPlugins()) {
    foreach (const sys::Function& function, plugin.functions()) {
      qDebug() << "function: " << function.name()
               << function.plugin().filePath();
    }
  }

  QList<FeatureTypes> featureTypes = getAvailableFeatureTypes();
  foreach (FeatureTypes featureType, featureTypes) {
    qDebug() << "featureType: " << static_cast<int>(featureType)
             << getFeatureTypeName(featureType);
  }

  QList<ElementTypes> elementTypes = getAvailableElementTypes();
  foreach (ElementTypes elementType, elementTypes) {
    qDebug() << "elementType: " << static_cast<int>(elementType)
             << getElementTypeName(elementType);
  }

  // start from a known-empty System layer. The test binary sits next to the
  // app's deployed config/, so a lazy Settings::init() would otherwise pick up
  // the real shipped defaults and make this test depend on ambient state
  Settings::loadSystemLayerFromFile(
      QString("%1/does-not-exist.json").arg(qApp->applicationDirPath()));

  QPair<QString, QString> function =
      sourceModel->getDefaultFunction(getFeatureTypeEnum("point"));
  QString name = Settings::get("defaultMeasurementConfig.point");
  qDebug() << "function: " << function.first << function.second
           << "measurementConfig: " << name;
  QVERIFY(function.first.isEmpty());
  QVERIFY(name.isEmpty());

  QString path = QString("%1/defaults.json").arg(SRCDIR);
  qDebug() << "path" << path;
  QVERIFY2(QFile::exists(path),
           qPrintable(QString("fixture missing: %1").arg(path)));

  Settings::loadSystemLayerFromFile(path);

  function = sourceModel->getDefaultFunction(getFeatureTypeEnum("point"));
  name = Settings::get("defaultMeasurementConfig.point");
  qDebug() << "function: " << function.first << function.second
           << "measurementConfig: " << name;

  QVERIFY(function.first == "function-fitpoint");
  QVERIFY(name == "FastPoint");
}

QTEST_MAIN(DialogsTest)

#include "dialogstest.moc"

/*!
 * The scalar-entity combo used to display persisted tokens ("point",
 * "circle") because CreateFeatureDialog parsed its visible text back into a
 * FeatureTypes with getFeatureTypeEnum(currentText()). That coupling meant
 * capitalising a label for the UI would silently change which type got
 * created - or create nothing at all.
 *
 * Items now show a display name and carry their type in Qt::UserRole. This
 * pins that: the visible text is a display name, resolution goes through the
 * data role, and - the property that actually matters - rewriting the visible
 * text does not change what the combo resolves to.
 *
 * The existing dialog cases deliberately bypass this combo ("type is set
 * directly"), so without this the whole path was untested.
 */
void DialogsTest::entityTypeComboResolvesByDataNotByVisibleText() {
  QStandardItemModel& model = ModelManager::getScalarEntityTypeNamesModel();
  QCOMPARE(model.rowCount(), 3);  // seeded by testInit: point, circle, plane

  const QList<FeatureTypes> expected = {FeatureTypes::ePointFeature,
                                        FeatureTypes::eCircleFeature,
                                        FeatureTypes::ePlaneFeature};

  for (int row = 0; row < model.rowCount(); row++) {
    QStandardItem* item = model.item(row);
    QVERIFY(item != nullptr);

    const FeatureTypes type = expected.at(row);

    // the item carries its type, so a consumer never parses the label
    QCOMPARE(static_cast<FeatureTypes>(item->data(Qt::UserRole).toInt()), type);

    // and what the user reads is the display name, not the persisted token
    QCOMPARE(item->text(), getFeatureTypeDisplayName(type));
  }

  // the property that matters: rewording a label must not change resolution
  QStandardItem* first = model.item(0);
  const FeatureTypes before =
      static_cast<FeatureTypes>(first->data(Qt::UserRole).toInt());
  first->setText(QStringLiteral("Totally Different Wording"));
  QCOMPARE(static_cast<FeatureTypes>(first->data(Qt::UserRole).toInt()),
           before);

  // restore, since the model is shared static state
  first->setText(getFeatureTypeDisplayName(before));
}

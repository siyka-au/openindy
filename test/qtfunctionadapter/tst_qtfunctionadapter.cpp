#include <QSignalSpy>
#include <QtTest>

#include "chooselalib.h"
#include "constructfunction.h"
#include "geometry/point.h"
#include "qtfunctionadapter.h"

using namespace oi;

namespace {
class TestFunction : public ConstructFunction, public Producer<Point> {
 public:
  explicit TestFunction() : ConstructFunction() {
    this->applicableFor.push_back(FeatureTypes::ePointFeature);
  }
  std::expected<Point, std::string> exec() override { return Point(); }
};
}  // namespace

class QtFunctionAdapterTest : public QObject {
  Q_OBJECT

 private slots:
  void initTestCase();

  void ofReturnsTheSameAdapterEachTime();
  void nullFunctionHasNoAdapter();
  void inputElementAndParameterChangesArriveAsSignals();
  void blockingTheAdapterSuppressesTheNotification();
  void adapterIsCleanedUpWithTheFunction();
};

void QtFunctionAdapterTest::initTestCase() {
  ChooseLALib::setLinearAlgebra(ChooseLALib::LinearAlgebraLibrary::Armadillo);
}

void QtFunctionAdapterTest::ofReturnsTheSameAdapterEachTime() {
  {
    TestFunction function;
    QtFunctionAdapter* first = QtFunctionAdapter::of(&function);
    QVERIFY(first != nullptr);
    QCOMPARE(QtFunctionAdapter::of(&function), first);
    QCOMPARE(QtFunctionAdapter::of(oi::Guarded<Function>(&function)), first);
    QCOMPARE(QtFunctionAdapter::registrySize(), 1);
  }
  QCOMPARE(QtFunctionAdapter::registrySize(), 0);
}

void QtFunctionAdapterTest::nullFunctionHasNoAdapter() {
  QVERIFY(QtFunctionAdapter::of(static_cast<Function*>(nullptr)) == nullptr);
  QVERIFY(QtFunctionAdapter::of(oi::Guarded<Function>()) == nullptr);
}

void QtFunctionAdapterTest::inputElementAndParameterChangesArriveAsSignals() {
  TestFunction function;
  QtFunctionAdapter* adapter = QtFunctionAdapter::of(&function);
  QSignalSpy inputSpy(adapter, &QtFunctionAdapter::inputElementsChanged);
  QSignalSpy paramSpy(adapter,
                      &QtFunctionAdapter::scalarInputParametersChanged);

  InputElement element(3);
  element.typeOfElement = ElementTypes::ePointElement;
  function.addInputElement(element, 0);
  QCOMPARE(inputSpy.count(), 1);
  function.removeInputElement(3, 0);
  QCOMPARE(inputSpy.count(), 2);
  QCOMPARE(paramSpy.count(), 0);

  function.setScalarInputParams(ScalarInputParams());
  QCOMPARE(paramSpy.count(), 1);
  QCOMPARE(inputSpy.count(), 2);
}

void QtFunctionAdapterTest::blockingTheAdapterSuppressesTheNotification() {
  TestFunction function;
  QtFunctionAdapter* adapter = QtFunctionAdapter::of(&function);
  QSignalSpy inputSpy(adapter, &QtFunctionAdapter::inputElementsChanged);

  // what the available-elements model does around a bulk change
  adapter->blockSignals(true);
  InputElement element(5);
  element.typeOfElement = ElementTypes::ePointElement;
  function.addInputElement(element, 0);
  function.addInputElement(InputElement(6), 0);
  adapter->blockSignals(false);
  QCOMPARE(inputSpy.count(), 0);
  emit adapter->inputElementsChanged();
  QCOMPARE(inputSpy.count(), 1);
}

void QtFunctionAdapterTest::adapterIsCleanedUpWithTheFunction() {
  auto* function = new TestFunction();
  QtFunctionAdapter::of(function);
  QCOMPARE(QtFunctionAdapter::registrySize(), 1);
  delete function;
  QCOMPARE(QtFunctionAdapter::registrySize(), 0);
}

QTEST_GUILESS_MAIN(QtFunctionAdapterTest)
#include "tst_qtfunctionadapter.moc"

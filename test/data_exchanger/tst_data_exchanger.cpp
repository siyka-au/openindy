#include <QSignalSpy>
#include <QThread>
#include <QtTest>
#include <atomic>

#include "data_exchanger.h"

using namespace oi;

/*!
 * An exchange plugin reports from its worker thread through the DataExchanger's
 * MessageSink/ExchangeObserver callbacks. Whatever thread they are called on,
 * the signals the UI listens to must be emitted on the exchanger's own (UI)
 * thread, in the order the plugin reported them.
 */
class DataExchangerTest : public QObject {
  Q_OBJECT

 private slots:
  void reportsFromAWorkerThreadArriveOnTheUiThreadInOrder();
  void reportsFromTheOwnThreadAreDeliveredDirectly();
  void failedExportIsReportedAsMessage();
};

void DataExchangerTest::reportsFromAWorkerThreadArriveOnTheUiThreadInOrder() {
  DataExchanger exchanger;
  QStringList events;
  QList<QThread*> threads;

  QObject::connect(
      &exchanger, &DataExchanger::sendMessage, &exchanger,
      [&](const QString& msg, const MessageTypes&, const MessageDestinations&) {
        events << "message:" + msg;
        threads << QThread::currentThread();
      });
  QObject::connect(&exchanger, &DataExchanger::updateNominalImportProgress,
                   &exchanger, [&](const int& progress, const QString& msg) {
                     events << QString("progress:%1:%2").arg(progress).arg(msg);
                     threads << QThread::currentThread();
                   });
  QObject::connect(&exchanger, &DataExchanger::nominalImportFinished,
                   &exchanger, [&](const bool& success) {
                     events << QString("finished:%1").arg(success);
                     threads << QThread::currentThread();
                   });

  QThread* worker = QThread::create([&exchanger] {
    // what a plugin does on its thread: message, progress, then done
    exchanger.onMessage("reading file", MessageTypes::eInformationMessage,
                        MessageDestinations::eConsoleMessage);
    exchanger.nominalImportProgress().onProgress({0.4, "40%"});
    exchanger.nominalImportProgress().onProgress({1.0, "done"});
    exchanger.onImportFinished(false);
  });
  worker->start();
  worker->wait();

  // nothing is delivered behind the UI thread's back
  QVERIFY(events.isEmpty());

  QTRY_COMPARE(events.size(), 4);
  QCOMPARE(events, (QStringList{"message:reading file", "progress:40:40%",
                                "progress:100:done", "finished:0"}));
  for (QThread* thread : std::as_const(threads)) {
    QCOMPARE(thread, QThread::currentThread());
  }
  delete worker;
}

void DataExchangerTest::reportsFromTheOwnThreadAreDeliveredDirectly() {
  DataExchanger exchanger;
  QSignalSpy progressSpy(&exchanger,
                         &DataExchanger::updateNominalImportProgress);
  exchanger.nominalImportProgress().onProgress({0.1, "now"});
  QCOMPARE(progressSpy.count(), 1);
  QCOMPARE(progressSpy.first().at(0).toInt(), 10);
}

void DataExchangerTest::failedExportIsReportedAsMessage() {
  DataExchanger exchanger;
  QSignalSpy messageSpy(&exchanger, &DataExchanger::sendMessage);
  QThread* worker =
      QThread::create([&exchanger] { exchanger.onExportFinished(false); });
  worker->start();
  worker->wait();
  QTRY_COMPARE(messageSpy.count(), 1);
  QVERIFY(messageSpy.first().at(0).toString().contains("not exported"));
  delete worker;
}

QTEST_GUILESS_MAIN(DataExchangerTest)
#include "tst_data_exchanger.moc"

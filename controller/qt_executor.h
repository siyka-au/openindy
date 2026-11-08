#ifndef OI_APP_CONTROLLER_QT_EXECUTOR_H_
#define OI_APP_CONTROLLER_QT_EXECUTOR_H_

#include <QObject>
#include <functional>

#include "executor.h"

/*!
 * The oi::Executor of the Qt application: runs every posted task on the thread
 * the executor object lives in (the UI thread, if it was created there) by
 * queueing it onto that thread's event loop. Tasks run in the order they were
 * posted; post() may be called from any thread.
 *
 * Sensor results are produced on the sensors' worker threads; handing them to
 * the stations through this executor delivers them on the UI thread in the
 * order the workers produced them, which is what the models and dialogs
 * expect. Tasks still queued when the executor is destroyed are discarded.
 */
class QtExecutor : public QObject, public oi::Executor {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(QtExecutor)

 public:
  explicit QtExecutor(QObject* parent = nullptr) : QObject(parent) {}

  void post(std::function<void()> task) override;
};

#endif  // OI_APP_CONTROLLER_QT_EXECUTOR_H_

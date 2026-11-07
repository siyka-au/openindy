#ifndef QTFUNCTIONADAPTER_H
#define QTFUNCTIONADAPTER_H

#include <QObject>
#include <QPointer>

#include "functionobserver.h"
#include "guarded.h"

namespace oi {
class Function;
}  // namespace oi

/*!
 * Turns the plain oi::FunctionObserver callbacks of a Function back into the
 * Qt signals the Qt application connects to. There is exactly one adapter per
 * function, created on first use by of() and destroyed together with the
 * function. The signals carry the names the function used to declare itself.
 *
 * Block the adapter's signals (blockSignals) to suppress the notifications
 * during a bulk change, exactly as blocking the function's own signals used
 * to.
 */
class QtFunctionAdapter : public QObject, public oi::FunctionObserver {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(QtFunctionAdapter)

 public:
  // the adapter of `function` (created and registered on first use), or
  // nullptr for a null function. The adapter lives in the function's thread.
  static QtFunctionAdapter* of(oi::Function* function);
  static QtFunctionAdapter* of(const oi::Guarded<oi::Function>& function);

  // number of live adapters (for tests)
  static int registrySize();

  ~QtFunctionAdapter() override;

 signals:

  void inputElementsChanged();
  void scalarInputParametersChanged();

 private:
  QtFunctionAdapter() = default;

  void onFunctionDestroyed(oi::Function* function) override;
  void onInputElementsChanged() override;
  void onScalarInputParametersChanged() override;
};

#endif  // QTFUNCTIONADAPTER_H

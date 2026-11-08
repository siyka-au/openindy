#ifndef OI_APP_CONTROLLER_TOOL_H_
#define OI_APP_CONTROLLER_TOOL_H_

#include <QCloseEvent>
#include <QList>
#include <QObject>
#include <QPointer>
#include <QWidget>

#include "guarded.h"
#include "oi_job.h"
#include "oi_request_response.h"
#include "plugin_metadata.h"
#include "types.h"

namespace oi {

class Tool : public QWidget {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(Tool)

 public:
  explicit Tool(QWidget* parent = nullptr);

  ~Tool() override;

  // ##########################
  // tool initialization method
  // ##########################

  virtual void init();

  virtual bool saveProjectEnabled();

  // #########################################
  // methods to get or set further information
  // #########################################

  [[nodiscard]] const PluginMetaData& getMetaData() const;

  [[nodiscard]] const ToolTypes& getToolType() const;

  [[nodiscard]] const oi::Guarded<OiJob>& getJob() const;
  void setJob(const oi::Guarded<OiJob>& job);

  // #########################
  // override QWidget methods
  // #########################

  void closeEvent(QCloseEvent* event) override;

 public slots:

  // #####################################
  // special events a tool could listen on
  // #####################################

  virtual void customRequest(OiRequestResponse* request);
  virtual void watchWindowKeyPressed(const Qt::Key& key);

  // receive emitted job
  virtual void setCurrentJob(const oi::Guarded<OiJob>& job);

 signals:

  // ############################################
  // signals to inform OpenIndy about tool issues
  // ############################################

  void sendMessage(const QString& msg, const MessageTypes& msgType,
                   const MessageDestinations& msgDest =
                       MessageDestinations::eConsoleMessage);
  void customResponse(OiRequestResponse* response);

  // ###############################
  // signals to start sensor actions
  // ###############################

  void startConnect();
  void startDisconnect();
  void startMeasurement();
  void startMove(const Reading& reading);
  void startAim();
  void startAimAndMeasure();
  void startToggleSight();
  void startInitialize();
  void startHome();
  void startCompensation();
  void startChangeMotorState();
  void startCustomAction(const QString& task);

 protected:
  // ###########################
  // input and output parameters
  // ###########################

  PluginMetaData metaData;

  ToolTypes toolType;

  oi::Guarded<OiJob> job;  // the OpenIndy job that this tool works on
};

}  // namespace oi

#ifndef STR
#define STR(x) #x
#endif
#ifndef STR2
#define STR2(x) STR(x)
#endif

#ifndef CONCAT
#define CONCAT(a, b) a##b
#endif
#ifndef CONCAT2
#define CONCAT2(a, b) CONCAT(a, b)
#endif

#include "plugin_abi.h"

#define TOOL_PREFIX de.openIndy.plugin.tool.v
#define Tool_iidd STR2(CONCAT2(TOOL_PREFIX, OI_PLUGIN_ABI_VERSION))

namespace oi {

/*!
 * \brief Interface a plugin library implements (next to oi::Plugin) when it
 * ships UI tools. The application obtains it by qobject_cast on the
 * QPluginLoader instance; core knows nothing about it.
 */
class ToolPlugin {
 public:
  virtual ~ToolPlugin() = default;

  virtual QList<QPointer<Tool> > createTools() = 0;
  virtual QPointer<Tool> createTool(const QString& name) = 0;
};

}  // namespace oi

Q_DECLARE_INTERFACE(oi::ToolPlugin, Tool_iidd)

#endif  // OI_APP_CONTROLLER_TOOL_H_

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

 private:
  // ###########################
  // input and output parameters
  // ###########################

  PluginMetaData metaData;

  ToolTypes toolType;

  oi::Guarded<OiJob> job;  // the OpenIndy job that this tool works on
};

}  // namespace oi

#include "plugin_abi.h"

inline const std::string Tool_iidd = oi::pluginIid("de.openIndy.plugin.tool.v");

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

// moc parses this declaration and needs the iid as a string literal, so the
// build passes it as a definition built from the same core ABI version
// (tst_pluginhost checks it equals Tool_iidd).
Q_DECLARE_INTERFACE(oi::ToolPlugin, OI_TOOL_IID)

#endif  // OI_APP_CONTROLLER_TOOL_H_

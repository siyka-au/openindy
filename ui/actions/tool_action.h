#ifndef OI_APP_UI_ACTIONS_TOOL_ACTION_H_
#define OI_APP_UI_ACTIONS_TOOL_ACTION_H_

#include <QAction>

/*!
 * \brief The ToolAction class
 */
class ToolAction : public QAction {
  Q_OBJECT

 public:
  explicit ToolAction(QObject* parent = nullptr);

  void setPluginName(const QString& pName);
  void setToolName(const QString& tName);

  [[nodiscard]] const QString& getPluginName() const;
  [[nodiscard]] const QString& getToolName() const;

 signals:
  void openToolWidget(const QString& pluginName, const QString& toolName);

 private:
  QString toolName;
  QString pluginName;

 private slots:
  void clickCallback();
};

#endif  // OI_APP_UI_ACTIONS_TOOL_ACTION_H_

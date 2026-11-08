#ifndef OI_APP_UI_WIDGETS_STATION_STATUS_H_
#define OI_APP_UI_WIDGETS_STATION_STATUS_H_

#include <QWidget>

namespace Ui {
class StationStatus;
}

class StationStatus : public QWidget {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(StationStatus)

 public:
  explicit StationStatus(QWidget* parent = nullptr);
  ~StationStatus() override;

  void setName(QString name);
  void setTemperature(QString temperature);
  void setBundled(bool bundled);

  void reset();

 private:
  Ui::StationStatus* ui;
};

#endif  // OI_APP_UI_WIDGETS_STATION_STATUS_H_

#ifndef OI_APP_CONTROLLER_EXCHANGE_PROJECT_EXCHANGER_H_
#define OI_APP_CONTROLLER_EXCHANGE_PROJECT_EXCHANGER_H_

#include <QByteArray>
#include <QPointer>
#include <QString>

#include "measurement_config_manager.h"
#include "oi_job.h"

using namespace oi;

/*!
 * \brief The ProjectExchanger class
 * Saves and loads OpenIndy projects. The format and the linking of a loaded
 * project's references live in the core (project_codec.h); this class gives the
 * core what only the application knows: where the plugins are and which
 * measurement configs exist.
 */
class ProjectExchanger {
 public:
  struct LoadResult {
    std::unique_ptr<OiJob> job;  //!< null if the project could not be loaded
    QString error;               //!< why, for the user
  };

  ProjectExchanger() = default;

  //! the project as bytes; empty if there is no job
  QByteArray saveProject(const oi::Guarded<OiJob>& job);

  //! the caller owns the job
  LoadResult loadProject(const QByteArray& project);

  QPointer<MeasurementConfigManager>& getMeasurementConfigManager();
  void setMeasurementConfigManager(
      const QPointer<MeasurementConfigManager>& mConfigManager);

 private:
  QPointer<MeasurementConfigManager> mConfigManager;
};

#endif  // OI_APP_CONTROLLER_EXCHANGE_PROJECT_EXCHANGER_H_

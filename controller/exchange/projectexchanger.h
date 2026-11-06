#ifndef OIPROJECTEXCHANGER_H
#define OIPROJECTEXCHANGER_H

#include <QByteArray>
#include <QPointer>
#include <QString>

#include "measurementconfigmanager.h"
#include "oijob.h"

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
    QPointer<OiJob> job;  //!< null if the project could not be loaded
    QString error;        //!< why, for the user
  };

  ProjectExchanger() = default;

  //! the project as bytes; empty if there is no job
  QByteArray saveProject(const QPointer<OiJob>& job);

  //! the caller owns the job
  LoadResult loadProject(const QByteArray& project);

  QPointer<MeasurementConfigManager>& getMeasurementConfigManager();
  void setMeasurementConfigManager(
      const QPointer<MeasurementConfigManager>& mConfigManager);

 private:
  QPointer<MeasurementConfigManager> mConfigManager;
};

#endif  // OIPROJECTEXCHANGER_H

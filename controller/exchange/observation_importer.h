#ifndef OBSERVATIONIMPORTER_H
#define OBSERVATIONIMPORTER_H

#include <QApplication>
#include <QObject>
#include <QString>
#include <QStringList>

#include "console.h"
#include "oi_job.h"
#include "progress.h"

using namespace oi;

/*!
 * \brief The ObservationImporter class
 * Imports observations into the active actual geometry
 */
class ObservationImporter : public QObject {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(ObservationImporter)

 public:
  explicit ObservationImporter(QObject* parent = nullptr);

  ~ObservationImporter() override;

  // #############################
  // get or set general attributes
  // #############################

  [[nodiscard]] const oi::Guarded<OiJob>& getCurrentJob() const;
  void setCurrentJob(const oi::Guarded<OiJob>& job);

  [[nodiscard]] const QString& getFileName() const;
  void setFileName(const QString& filename);

  // where the import reports how far it is (non-owning, may be null; it is
  // called from the thread the import runs on)
  void setProgressSink(oi::ProgressSink* sink);

 public slots:

  // ######################
  // start import or export
  // ######################

  bool importObservations();

 signals:

  // ##############################################
  // signals to inform OpenIndy about import issues
  // ##############################################

  void sendMessage(const QString& msg, const MessageTypes& msgType,
                   const MessageDestinations& msgDest =
                       MessageDestinations::eConsoleMessage);
  void importFinished(const bool& success);

 private:
  // ##################
  // general attributes
  // ##################

  oi::Guarded<OiJob> currentJob;

  QString filename;
  oi::ProgressSink* progressSink = nullptr;
};

#endif  // OBSERVATIONIMPORTER_H

#ifndef OIDATAEXCHANGER_H
#define OIDATAEXCHANGER_H

#include <QApplication>
#include <QObject>
#include <QString>
#include <QStringList>
#include <functional>

#include "console.h"
#include "exchangeParams.h"
#include "exchangedefinedformat.h"
#include "exchangeobserver.h"
#include "exchangesimpleascii.h"
#include "measurementconfigmanager.h"
#include "messagesink.h"
#include "observationimporter.h"
#include "oijob.h"
#include "pluginloader.h"
#include "pluginrepository.h"
#include "settings.h"
#include "util.h"

using namespace oi;

/*!
 * \brief The DataExchanger class
 * Imports or exports nominals by invoking plugins
 */
class DataExchanger : public QObject,
                      public oi::MessageSink,
                      public oi::ExchangeObserver {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(DataExchanger)

 public:
  explicit DataExchanger(QObject* parent = nullptr);

  ~DataExchanger() override;

  // #############################
  // get or set general attributes
  // #############################

  [[nodiscard]] const QPointer<OiJob>& getCurrentJob() const;
  void setCurrentJob(const QPointer<OiJob>& job);

  const QPointer<MeasurementConfigManager>& getMeasurementConfigManager();
  void setMesaurementConfigManager(
      const QPointer<MeasurementConfigManager>& manager);

  void setExchangeParams(ExchangeParams exchangeParams);

  // ######################
  // start import or export
  // ######################

  // import or export features
  bool importData(const ExchangeParams& params);
  bool exportData(const ExchangeParams& params);

  // import observations
  bool importObservations(const QString& filename);

  // ##############################################################
  // the exchange plugin reports from its worker thread through plain
  // callbacks (it is given this object as sink and observer); these hand
  // every report over to this object's (the UI) thread, in order, before it
  // is turned into signals and slots. Public so they are the seam tests use.
  // ##############################################################

  void onMessage(const QString& msg, const MessageTypes& type,
                 const MessageDestinations& dest) override;
  void onProgress(int percent, const QString& message) override;
  void onImportFinished(bool success) override;
  void onExportFinished(bool success) override;

 signals:

  // ##############################
  // inform about exchange progress
  // ##############################

  // nominal im- and export
  void updateNominalImportProgress(const int& progress,
                                   const QString& msg);  // 0 <= progress <= 100
  void updateNominalExportProgress(const int& progress,
                                   const QString& msg);  // 0 <= progress <= 100
  void nominalImportFinished(const bool& success);
  void nominalExportFinished(const bool& success);

  // observation import
  void updateObservationImportProgress(
      const int& progress, const QString& msg);  // 0 <= progress <= 100
  void observationImportFinished(const bool& success);

  // print messages
  void sendMessage(const QString& msg, const MessageTypes& msgType,
                   const MessageDestinations& msgDest =
                       MessageDestinations::eConsoleMessage);

 private slots:

  // ########################################################
  // add features to current job after an import was finished
  // ########################################################

  void importFeatures(const bool& success);
  void exportFeatures(const bool& success);

  void importObservationsFinished(const bool& success);

 private:
  // runs `work` on this object's thread: directly if called from it, queued
  // otherwise (what a signal emitted by the plugin used to do)
  void runOnOwnThread(std::function<void()> work);

  // ##################
  // general attributes
  // ##################

  QPointer<OiJob> currentJob;

  ExchangeParams exchangeParams;
  QPointer<ExchangeInterface> exchange;
  bool exporting = false;  // whether the running exchange is an export

  QPointer<ObservationImporter> observationImporter;

  // #########################
  // thread the plugin runs on
  // #########################

  QThread exchangeThread;

  QPointer<MeasurementConfigManager> mConfigManager;

  // #############
  // helper method
  // #############
  void addFunctionsAndMConfigs(const QList<QPointer<FeatureWrapper> >& actuals,
                               const MeasurementConfig& mConfig,
                               const QString& path, const QString& fName);

  // import readings of features
  void importMeasurements(QList<QPointer<FeatureWrapper> > features);
  void createActuals(QList<QPointer<FeatureWrapper> > features);
};

#endif  // OIDATAEXCHANGER_H

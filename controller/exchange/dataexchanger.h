#ifndef OIDATAEXCHANGER_H
#define OIDATAEXCHANGER_H

#include <QApplication>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QtCore>
#include <fstream>
#include <functional>
#include <memory>
#include <vector>

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

  [[nodiscard]] const oi::Guarded<OiJob>& getCurrentJob() const;
  void setCurrentJob(const oi::Guarded<OiJob>& job);

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

  void onMessage(const std::string& msg, const MessageTypes& type,
                 const MessageDestinations& dest) override;
  void onProgress(int percent, const std::string& message) override;
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

  // runs `work` once on exchangeThread; replaces any earlier runner
  void runOnExchangeThread(std::function<void()> work);
  void discardExchangeRunner();

  // ##################
  // general attributes
  // ##################

  oi::Guarded<OiJob> currentJob;

  ExchangeParams exchangeParams;
  oi::Guarded<ExchangeInterface> exchange;
  bool exporting = false;  // whether the running exchange is an export

  QPointer<ObservationImporter> observationImporter;

  // #########################
  // thread the plugin runs on
  // #########################

  // declared before the thread so the thread is gone before the stream
  std::unique_ptr<std::fstream> exchangeStream;  // the exchange's file
  QThread exchangeThread;
  QObject* exchangeRunner{nullptr};  // context object living on exchangeThread

  QPointer<MeasurementConfigManager> mConfigManager;

  // #############
  // helper method
  // #############
  void addFunctionsAndMConfigs(
      const std::vector<oi::Guarded<FeatureWrapper> >& actuals,
      const MeasurementConfig& mConfig, const QString& path,
      const QString& fName);

  // import readings of features
  void importMeasurements(std::vector<oi::Guarded<FeatureWrapper> > features);
  void createActuals(std::vector<oi::Guarded<FeatureWrapper> > features);
};

#endif  // OIDATAEXCHANGER_H

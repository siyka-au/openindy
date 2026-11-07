#include "observationimporter.h"

#include <QRegularExpression>
#include <QtCore>
#include <algorithm>
#include <array>
#include <vector>

/*!
 * \brief ObservationImporter::ObservationImporter
 * \param parent
 */
ObservationImporter::ObservationImporter(QObject* parent) : QObject(parent) {}

/*!
 * \brief ObservationImporter::~DataExchanger
 */
ObservationImporter::~ObservationImporter() {}

/*!
 * \brief ObservationImporter::getCurrentJob
 * \return
 */
const oi::Guarded<OiJob>& ObservationImporter::getCurrentJob() const {
  return this->currentJob;
}

/*!
 * \brief ObservationImporter::setCurrentJob
 * \param job
 */
void ObservationImporter::setCurrentJob(const oi::Guarded<OiJob>& job) {
  this->currentJob = job;
}

/*!
 * \brief ObservationImporter::getFileName
 * \return
 */
const QString& ObservationImporter::getFileName() const {
  return this->filename;
}

/*!
 * \brief ObservationImporter::setFileName
 * \param filename
 */
void ObservationImporter::setFileName(const QString& filename) {
  this->filename = filename;
}

/*!
 * \brief ObservationImporter::importObservations
 * \return
 */
bool ObservationImporter::importObservations() {
  // check job
  if (this->currentJob.isNull()) {
    emit this->sendMessage("No active job", MessageTypes::eErrorMessage,
                           MessageDestinations::eMessageBoxMessage);
    emit this->importFinished(false);
    return false;
  }

  // check active station
  if (this->currentJob->getActiveStation().isNull()) {
    emit this->sendMessage("No active station", MessageTypes::eErrorMessage,
                           MessageDestinations::eMessageBoxMessage);
    emit this->importFinished(false);
    return false;
  }

  // check active feature
  if (this->currentJob->getActiveFeature().isNull() ||
      this->currentJob->getActiveFeature()->getGeometry().isNull() ||
      !this->currentJob->getActiveFeature()
           ->getGeometry()
           ->getOwnerDescription()
           .has_value()) {
    emit this->sendMessage(
        "You have to select an actual geometry before importing observations",
        MessageTypes::eErrorMessage, MessageDestinations::eMessageBoxMessage);
    emit this->importFinished(false);
    return false;
  }

  // check filename
  if (this->filename.compare("") == 0) {
    emit this->sendMessage("No file specified", MessageTypes::eErrorMessage,
                           MessageDestinations::eMessageBoxMessage);
    emit this->importFinished(false);
    return false;
  }

  // create and check device
  QPointer<QFile> device = new QFile(this->filename);
  QFileInfo fileInfo(*device.data());
  if (!fileInfo.exists()) {
    emit this->sendMessage(
        "The selected file cannot be found in the file system",
        MessageTypes::eErrorMessage, MessageDestinations::eMessageBoxMessage);
    emit this->importFinished(false);
    return false;
  }

  // create list of imported readings
  std::vector<oi::Guarded<Reading> > readings;

  // open file and create observations
  try {
    // open device
    device->open(QIODevice::ReadOnly | QIODevice::Text);

    // init helper variables
    const oi::OperationContext ctx(this->progressSink);
    qint64 fileSize = device->size();
    qint64 readSize = 0;
    qint64 numObservations = 0;

    const auto curDateTime = std::chrono::system_clock::now();
    // read all lines
    QTextStream in(device);
    while (!in.atEnd()) {
      // get current line
      QString line = in.readLine();
      readSize += line.size();

      if (line.startsWith("#")         // skip comment
          || line.startsWith(";")      // skip comment
          || line.trimmed().isEmpty()  // skip empty lines
      ) {
        continue;
      }

      // split the line at whitespaces and check number of columns
      QStringList columns =
          line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
      if (columns.size() != 3) {
        continue;
      }

      // check wether the imported values can be parsed to double
      bool parsingSuccess = true;

      // parse the line to xyz coordinates
      std::array<double, 3> xyz{};
      for (int i = 0; i < 3; i++) {
        xyz[i] = columns.at(i).toDouble(&parsingSuccess);
        if (!parsingSuccess) {
          break;
        }
      }

      // check if there was an error
      if (!parsingSuccess) {
        continue;
      }

      // create reading
      ReadingCartesian rCartesian;
      rCartesian.xyz.setAt(0, xyz[0]);
      rCartesian.xyz.setAt(1, xyz[1]);
      rCartesian.xyz.setAt(2, xyz[2]);
      rCartesian.isValid = true;
      oi::Guarded<Reading> reading = new Reading(rCartesian);
      reading->setSensorFace(SensorFaces::eUndefinedSide);
      reading->setMeasuredAt(curDateTime);
      reading->setImported(true);

      // add reading to list of imported readings
      readings.push_back(reading);

      // update import progress
      numObservations++;
      // never 100% before the import has finished
      ctx.report(fileSize > 0
                     ? std::min(0.99, static_cast<double>(readSize) /
                                          static_cast<double>(fileSize))
                     : 0.0,
                 QString("%1 observation(s) loaded")
                     .arg(numObservations)
                     .toStdString());
      readSize += 2;
    }

    device->close();
    delete device;

  } catch (const exception& e) {
    emit this->sendMessage("Error while opening file",
                           MessageTypes::eErrorMessage,
                           MessageDestinations::eMessageBoxMessage);
    emit this->importFinished(false);
    return false;
  }

  // add observations to the active feature
  this->currentJob->addMeasurementResults(
      this->currentJob->getActiveFeature()->getGeometry()->getId(), readings);

  emit this->importFinished(true);

  return true;
}

void ObservationImporter::setProgressSink(oi::ProgressSink* sink) {
  this->progressSink = sink;
}

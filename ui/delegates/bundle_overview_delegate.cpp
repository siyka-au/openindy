#include "bundle_overview_delegate.h"

#include <vector>

#include "qt_convert.h"

namespace {
constexpr int kScaleDigits = 6;  // decimals shown for the bundle scale
// text layout of one overview cell in pixels
constexpr int kTextLeft = 5;
constexpr int kRowTitle = 20;
constexpr int kRowMaxError = 40;
constexpr int kRowStdDev = 60;
constexpr int kRowScale = 80;
constexpr int kRowCommonGeometries = 100;
constexpr int kSizeHintWidth = 100;
constexpr int kSizeHintHeight = 120;
}  // namespace

BundleOverviewDelegate::BundleOverviewDelegate(QObject* parent)
    : QAbstractItemDelegate(parent) {}
void BundleOverviewDelegate::paint(QPainter* painter,
                                   const QStyleOptionViewItem& option,
                                   const QModelIndex& index) const {
  const QString stationName = index.data(Qt::DisplayRole).toString();

  const BundleGeometriesModel* model =
      static_cast<const BundleGeometriesModel*>(index.model());
  const oi::Guarded<OiJob> job = model->getCurrentJob();
  if (job.isNull()) {
    return;
  }

  std::vector<oi::Guarded<FeatureWrapper> > stations =
      job->getFeaturesByName(toStd(stationName));
  if (stations.empty()) {
    return;
  }
  oi::Guarded<Station> station = stations.front()->getStation();
  if (station.isNull()) {
    return;
  }

  QString scaleS = "";
  double scale = 0;
  bool isOk = false;  // show green or red bullet point
  bool isSolved = false;
  for (oi::Guarded<TrafoParam> tp :
       model->getCurrentJob()->getTransformationParametersList()) {
    if (isBundleStationTrafo(tp) &&
        tp->getDestinationSystem()->getFeatureName() == stationName &&
        tp->getStartSystem()->getFeatureName() == "Bundle01") {
      scaleS = toQString(tp->getDisplayScaleX(kScaleDigits));
      scale = tp->getScale().getAt(0);  // X
      isSolved = tp->getIsSolved();
      break;
    }
  }
  isOk = isSolved;

  int numberOfCommonGeometries = 0;
  double maxError = 0;  // max v
  QString maxErrorGeometry = "";
  double stdDev = 0.0;
  if (isSolved) {
    double sum_vv = 0.0;
    int count_v = 0;

    for (oi::Guarded<Geometry> tg : station->getTargetGeometries()) {
      if (!tg.isNull() && tg->getIsCommon() && tg->getIsSolved()) {
        numberOfCommonGeometries++;

        OiVec actual = tg->getPosition().getVector();
        for (oi::Guarded<Observation> observation : tg->getObservations()) {
          if (observation->getStation()->getId() == station->getId()) {
            // see ObservationModel::data -> case eObservationDisplayIsUsed:
            if (tg->getFunctions().size() > 0 &&
                !tg->getFunctions().at(0).isNull()) {
              oi::Guarded<Function> function =
                  tg->getFunctions()[0];  // first function BestFit...

              if (!function->getShouldBeUsed(0, observation->getId())) {
                continue;
              }

              count_v++;
              OiVec xyz = observation->getXYZ();
              xyz.removeLast();
              double v = (xyz - actual).length();

              if (v > maxError) {
                maxError = v;
                maxErrorGeometry = toQString(tg->getFeatureName());
              }

              sum_vv += v * v;
            }
          }
        }
      }
    }
    stdDev = sqrt(sum_vv / (count_v - 1));
  }

  /* TODO later
      bool hasTransformationBundleToPart = false;
      const double TEMPERATURE_NOT_SET = -1000.;
      double actualTemperature = TEMPERATURE_NOT_SET;
      double referenceTemperature = TEMPERATURE_NOT_SET;
      QString material;
      for(oi::Guarded<TrafoParam> tp :
     model->getCurrentJob()->getTransformationParametersList()) { if(
     !isBundleStationTrafo(tp)
              && tp->getDestinationSystem()->getFeatureName() == "PART"
              && tp->getStartSystem()->getFeatureName() == "Bundle01"
              && tp->getIsSolved()
              && tp->getIsUsed()) {
                  hasTransformationBundleToPart = true;

                  for(oi::Guarded<Function> f : tp->getFunctions()) { // search
     for alignment with temperature material =
     f->getStringParameter().value("material"); actualTemperature =
     f->getDoubleParameter().value("actual temperature", TEMPERATURE_NOT_SET);
                      referenceTemperature =
     f->getDoubleParameter().value("reference temperature",
     TEMPERATURE_NOT_SET); if(actualTemperature > TEMPERATURE_NOT_SET) { break;
                      }
                  }
                  break;
          }
      }
  */

  isOk = convertFromDefault(maxError, UnitType::eUnitMilliMeter) <=
                     this->maxError  // convert from [m] to [mm]
                 && convertFromDefault(stdDev, UnitType::eUnitMilliMeter) <=
                        this->maxStdDev  // convert from [m] to [mm]
                 && abs(1. - scale) <= this->maxScaleDev &&
                 numberOfCommonGeometries >= this->minCommonPoints
             ? isOk
             : false;

  /*
   *  UI
   */
  painter->setPen(Qt::black);
  painter->drawText(option.rect.topLeft() += QPoint(kTextLeft, kRowTitle),
                    QString("%1").arg(stationName));
  if (isSolved) {
    painter->drawText(
        option.rect.topLeft() += QPoint(kTextLeft, kRowMaxError),
        QString("Max. Error:\t\t%1 [mm]\t%2")
            .arg(QString::number(
                convertFromDefault(maxError, UnitType::eUnitMilliMeter), 'f',
                3))
            .arg(maxErrorGeometry));

    painter->drawText(
        option.rect.topLeft() += QPoint(kTextLeft, kRowStdDev),
        QString("StdDev:\t\t%1 [mm]")
            .arg(QString::number(
                convertFromDefault(stdDev, UnitType::eUnitMilliMeter), 'f',
                3)));

    /* TODO later
    QString temperature;
    if(hasTransformationBundleToPart && actualTemperature > TEMPERATURE_NOT_SET)
    { temperature = QString(" (%2°C %3)").arg(actualTemperature).arg(material);
    }*/
    painter->drawText(option.rect.topLeft() += QPoint(kTextLeft, kRowScale),
                      QString("Scale:\t\t%1%2")
                          .arg(scaleS)
                          .arg("" /* TODO later temperature */));

    painter->drawText(
        option.rect.topLeft() += QPoint(kTextLeft, kRowCommonGeometries),
        QString("Common Geometries:\t%1").arg(numberOfCommonGeometries));
  }
  painter->setPen(isOk ? Qt::green : Qt::red);
  painter->drawText(option.rect.topLeft() += QPoint(kTextLeft, kRowTitle),
                    QString("\t\t%1").arg("⬤"));
}
// BundleParameterTableProxyModel::isBundleStationTrafo
bool BundleOverviewDelegate::isBundleStationTrafo(
    const oi::Guarded<TrafoParam>& param) const {
  return (param->getStartSystem()->getIsStationSystem() &&
          param->getDestinationSystem()->getIsBundleSystem()) ||
         (param->getStartSystem()->getIsBundleSystem() &&
          param->getDestinationSystem()->getIsStationSystem());
}
QSize BundleOverviewDelegate::sizeHint(const QStyleOptionViewItem& option,
                                       const QModelIndex& index) const {
  return QSize(kSizeHintWidth, kSizeHintHeight);
}

QWidget* BundleOverviewDelegate::createEditor(
    QWidget* parent, const QStyleOptionViewItem& option,
    const QModelIndex& index) const {
  return nullptr;
}

void BundleOverviewDelegate::setEditorData(QWidget* editor,
                                           const QModelIndex& index) const {}

void BundleOverviewDelegate::setModelData(QWidget* editor,
                                          QAbstractItemModel* model,
                                          const QModelIndex& index) const {}

void BundleOverviewDelegate::updateEditorGeometry(
    QWidget* editor, const QStyleOptionViewItem& option,
    const QModelIndex& index) const {}

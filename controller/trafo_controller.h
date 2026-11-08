#ifndef OI_APP_CONTROLLER_TRAFO_CONTROLLER_H_
#define OI_APP_CONTROLLER_TRAFO_CONTROLLER_H_

#include <QObject>
#include <QPointer>

#include "function.h"
#include "observation.h"

using namespace oi;

/*!
 * \brief The TrafoController class
 *
 */
class TrafoController : public QObject {
  Q_OBJECT

 public:
  explicit TrafoController(QObject* parent = nullptr);

 signals:

 public slots:

  // ######################
  // transform observations
  // ######################

  void transformObservations(
      const oi::Guarded<Feature>& feature,
      const oi::Guarded<CoordinateSystem>& destinationSystem);
  void transformObservations(
      const oi::Guarded<CoordinateSystem>& startSystem,
      const oi::Guarded<CoordinateSystem>& destinationSystem);
  void transformCoordSystems(
      const oi::Guarded<CoordinateSystem>& startSystem,
      const oi::Guarded<CoordinateSystem>& destinationSystem, bool isStation);

  bool getTransformationMatrix(
      OiMat& trafoMat, const oi::Guarded<CoordinateSystem>& startSystem,
      const oi::Guarded<CoordinateSystem>& destinationSystem);

 private:
  // ##############
  // helper methods
  // ##############

  oi::Guarded<TrafoParam> findTransformation(
      const oi::Guarded<CoordinateSystem>& startSystem,
      const oi::Guarded<CoordinateSystem>& destinationSystem);
  void setDefaultOriginAndNormal(
      const oi::Guarded<CoordinateSystem>& startSystem, bool isStation);
};

#endif  // OI_APP_CONTROLLER_TRAFO_CONTROLLER_H_

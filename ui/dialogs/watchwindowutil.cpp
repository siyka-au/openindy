#include "watchwindowutil.h"

#include <math.h>

#include <QtCore>

using namespace oi;

WatchWindowUtil::WatchWindowUtil() {}

Result WatchWindowUtil::getPosition(oi::Guarded<FeatureWrapper> feature,
                                    OiVec trackerXYZ) {
  Result result;
  result.position = Position::NullObject;
  result.radius = Radius();
  result.delta = OiVec(trackerXYZ.getSize());
  result.d3D = -1.0;

  if (feature.isNull()) {
    return result;
    // check if current feature is a solved geometry with position
  } else if (!feature->getGeometry().isNull() &&
             feature->getGeometry()->hasPosition() &&
             feature->getGeometry()->getIsSolved()) {
    // get radius
    switch (feature->getFeatureTypeEnum()) {
      case FeatureTypes::eCircleFeature:
        result.radius = feature->getCircle()->getRadius();
        break;
      case FeatureTypes::eCylinderFeature:
        result.radius = feature->getCylinder()->getRadius();
        break;
      default:
        result.radius = Radius();
    }

    // get position
    oi::Guarded<Geometry> geometry = feature->getGeometry();
    switch (feature->getFeatureTypeEnum()) {
      case FeatureTypes::ePlaneFeature: {
        double dot = NAN;
        OiVec::dot(dot, trackerXYZ - geometry->getPosition().getVectorH(),
                   geometry->getDirection().getVectorH());
        OiVec point = trackerXYZ - dot * geometry->getDirection().getVectorH();
        result.position =
            Position(point.getAt(0), point.getAt(1), point.getAt(2));
        break;
      }
      case FeatureTypes::eCircleFeature:
      case FeatureTypes::eCylinderFeature:
      case FeatureTypes::eLineFeature: {
        // projection on axis
        double dot = NAN;
        OiVec::dot(dot, trackerXYZ - geometry->getPosition().getVectorH(),
                   geometry->getDirection().getVectorH());
        OiVec point = geometry->getPosition().getVectorH() +
                      dot * geometry->getDirection().getVectorH();
        result.position =
            Position(point.getAt(0), point.getAt(1), point.getAt(2));
        break;
      }
      default:
        result.position = geometry->getPosition();
    }

    // check if active feature is a coordinate system
  } else if (!feature->getCoordinateSystem().isNull()) {
    result.position = feature->getCoordinateSystem()->getOrigin();

    // check if active feature is a station
  } else if (!feature->getStation().isNull()) {
    result.position = feature->getStation()->getPosition()->getPosition();
  }

  if (!result.position.isNull()) {
    result.delta = trackerXYZ - result.position.getVectorH();
    result.d3D = qSqrt(result.delta.getAt(0) * result.delta.getAt(0) +
                       result.delta.getAt(1) * result.delta.getAt(1) +
                       result.delta.getAt(2) * result.delta.getAt(2)) -
                 result.radius.getRadius();

    if (feature->getFeatureTypeEnum() == FeatureTypes::ePlaneFeature) {
      double dot = NAN;
      OiVec d = OiVec(result.delta.getSize());
      d = result.delta;
      d.removeLast();  // xyz
      OiVec::dot(dot, d, feature->getGeometry()->getDirection().getVector());
      result.d3D *= (dot > 0. ? 1.0 : -1.0);
    }
  }
  return result;
}

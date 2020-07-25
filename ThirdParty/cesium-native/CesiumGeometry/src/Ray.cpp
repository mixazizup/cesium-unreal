#include "CesiumGeometry/Ray.h"

namespace CesiumGeometry {

    Ray::Ray(const glm::dvec3& origin, const glm::dvec3& direction) :
        _origin(origin),
        _direction(direction)
    {
    }

    Ray Ray::operator-() const {
        return Ray(this->_origin, -this->_direction);
    }

}

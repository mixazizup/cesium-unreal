#include "Cesium3DTiles/BoundingVolume.h"
#include <algorithm>

namespace Cesium3DTiles {

    BoundingVolume transformBoundingVolume(const glm::dmat4x4& transform, const BoundingVolume& boundingVolume) {
        switch (boundingVolume.index()) {
        case 0:
        {
            const BoundingBox& boundingBox = std::get<BoundingBox>(boundingVolume);
            glm::dvec3 center = transform * glm::dvec4(boundingBox.center, 1.0);
            glm::dmat3 halfAxes = glm::dmat3(transform) * boundingBox.halfAxes;
            return BoundingBox(center, halfAxes);
        }
        case 1:
        {
            // Regions are not transformed.
            return boundingVolume;
        }
        case 2:
        {
            const BoundingSphere& boundingSphere = std::get<BoundingSphere>(boundingVolume);
            glm::dvec3 center = transform * glm::dvec4(boundingSphere.center, 1.0);

            double uniformScale = std::max(
                std::max(
                    glm::length(glm::dvec3(transform[0])),
                    glm::length(glm::dvec3(transform[1]))
                ),
                glm::length(glm::dvec3(transform[2]))
            );

            return BoundingSphere(center, boundingSphere.radius * uniformScale);
        }
        default:
            return boundingVolume;
        }
    }

}

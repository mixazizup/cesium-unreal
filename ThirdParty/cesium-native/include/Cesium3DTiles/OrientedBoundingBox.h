#pragma once

#include <glm/vec3.hpp>
#include <glm/mat3x3.hpp>
#include "Cesium3DTiles/CullingResult.h"
#include "Cesium3DTiles/Ellipsoid.h"
#include "Cesium3DTiles/Rectangle.h"

namespace Cesium3DTiles {

    class Plane;

    class OrientedBoundingBox {
    public:
        static OrientedBoundingBox fromRectangle(
            const Rectangle& rectangle,
            double minimumHeight = 0.0,
            double maximumHeight = 0.0,
            const Ellipsoid& ellipsoid = Ellipsoid::WGS84
        );

        OrientedBoundingBox(
            const glm::dvec3& center,
            const glm::dmat3& halfAxes
        ) :
            _center(center),
            _halfAxes(halfAxes)
        {}

        /**
         * @brief Get the Center object
         * 
         * @return const glm::dvec3& 
         */
        const glm::dvec3& getCenter() const { return this->_center; }
        const glm::dmat3& getHalfAxes() const { return this->_halfAxes; }

        /// <summary>
        /// Determines on which side of a plane the bounding box is located.
        /// </summary>
        /// <param name="plane">The plane to test.</param>
        /// <returns>
        /// <see cref="CullingResult::Inside"/> if the entire box is on the side of the plane
        /// the normal is pointing, <see cref="CullingResult::Outside"/> if the entire box is
        /// on the opposite side, and <see cref="CullingResult::Intersecting"/> if the box
        /// intersects the plane.
        /// </returns>
        CullingResult intersectPlane(const Plane& plane) const;

        /// <summary>
        /// Computes the distance squared from a given position to the closest
        /// point on this bounding volume. The bounding volume and the position
        /// must be expressed in the same coordinate system.
        /// </summary>
        /// <param name="position">The position.</param>
        /// <returns>The squared distance.</returns>
        double computeDistanceSquaredToPosition(const glm::dvec3& position) const;

    private:
        glm::dvec3 _center;
        glm::dmat3 _halfAxes;
    };

}

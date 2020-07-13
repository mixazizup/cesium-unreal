#define _USE_MATH_DEFINES
#include <cmath>
#include <optional>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>
#include "catch2/catch.hpp"
#include "Cesium3DTiles/BoundingBox.h"
#include "Cesium3DTiles/Camera.h"

TEST_CASE("BoundingBox::intersectPlane") {
    struct TestCase {
        glm::dvec3 center;
        glm::dmat3 axes;
    };

    auto testCase = GENERATE(
        // untransformed
        TestCase { glm::dvec3(0.0), glm::dmat3(1.0) },
        // off-center
        TestCase { glm::dvec3(1.0, 0.0, 0.0), glm::dmat3(1.0) },
        TestCase { glm::dvec3(0.7, -1.8, 12.0), glm::dmat3(1.0) },
        // rotated
        TestCase { glm::dvec3(0.0), glm::rotate(glm::dmat4(), 1.2, glm::dvec3(0.5, 1.5, -1.2)) },
        // scaled
        TestCase { glm::dvec3(0.0), glm::scale(glm::dmat4(), glm::dvec3(1.5, 0.4, 20.6)) },
        TestCase { glm::dvec3(0.0), glm::scale(glm::dmat4(), glm::dvec3(0.0, 0.4, 20.6)) },
        TestCase { glm::dvec3(0.0), glm::scale(glm::dmat4(), glm::dvec3(1.5, 0.0, 20.6)) },
        TestCase { glm::dvec3(0.0), glm::scale(glm::dmat4(), glm::dvec3(1.5, 0.4, 0.0)) },
        TestCase { glm::dvec3(0.0), glm::scale(glm::dmat4(), glm::dvec3(0.0, 0.0, 0.0)) },
        // arbitrary box
        TestCase {
            glm::dvec3(-5.1, 0.0, 0.1),
            glm::rotate(
                glm::scale(glm::dmat4(), glm::dvec3(1.5, 80.4, 2.6)),
                1.2,
                glm::dvec3(0.5, 1.5, -1.2)
            )
        }
    );

    SECTION("test corners, edges, faces") {
        const double SQRT1_2 = pow(1.0 / 2.0, 1 / 2.0);
        const double SQRT3_4 = pow(3.0 / 4.0, 1 / 2.0);

        auto box = Cesium3DTiles::BoundingBox(testCase.center, testCase.axes * 0.5);
        
        std::string s = glm::to_string(box.halfAxes);

        auto planeNormXform = [&testCase](double nx, double ny, double nz, double dist) {
            auto n = glm::dvec3(nx, ny, nz);
            auto arb = glm::dvec3(357, 924, 258);
            auto p0 = glm::normalize(n) * -dist;
            auto tang = glm::normalize(glm::cross(n, arb));
            auto binorm = glm::normalize(glm::cross(n, tang));

            p0 = testCase.axes * p0;
            tang = testCase.axes * tang;
            binorm = testCase.axes * binorm;

            n = glm::cross(tang, binorm);
            if (glm::length(n) == 0.0) {
                return std::optional<Cesium3DTiles::Plane>();
            }
            n = glm::normalize(n);

            p0 += testCase.center;
            double d = -glm::dot(p0, n);
            if (abs(d) > 0.0001 && glm::dot(n, n) > 0.0001) {
                return std::optional(Cesium3DTiles::Plane(n, d));
            }

            return std::optional<Cesium3DTiles::Plane>();
        };

        std::optional<Cesium3DTiles::Plane> pl;

        // Tests against faces

        pl = planeNormXform(+1.0, +0.0, +0.0, 0.50001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Inside));
        pl = planeNormXform(-1.0, +0.0, +0.0, 0.50001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Inside));
        pl = planeNormXform(+0.0, +1.0, +0.0, 0.50001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Inside));
        pl = planeNormXform(+0.0, -1.0, +0.0, 0.50001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Inside));
        pl = planeNormXform(+0.0, +0.0, +1.0, 0.50001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Inside));
        pl = planeNormXform(+0.0, +0.0, -1.0, 0.50001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Inside));

        pl = planeNormXform(+1.0, +0.0, +0.0, 0.49999);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Intersecting));
        pl = planeNormXform(-1.0, +0.0, +0.0, 0.49999);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Intersecting));
        pl = planeNormXform(+0.0, +1.0, +0.0, 0.49999);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Intersecting));
        pl = planeNormXform(+0.0, -1.0, +0.0, 0.49999);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Intersecting));
        pl = planeNormXform(+0.0, +0.0, +1.0, 0.49999);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Intersecting));
        pl = planeNormXform(+0.0, +0.0, -1.0, 0.49999);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Intersecting));

        pl = planeNormXform(+1.0, +0.0, +0.0, -0.49999);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Intersecting));
        pl = planeNormXform(-1.0, +0.0, +0.0, -0.49999);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Intersecting));
        pl = planeNormXform(+0.0, +1.0, +0.0, -0.49999);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Intersecting));
        pl = planeNormXform(+0.0, -1.0, +0.0, -0.49999);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Intersecting));
        pl = planeNormXform(+0.0, +0.0, +1.0, -0.49999);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Intersecting));
        pl = planeNormXform(+0.0, +0.0, -1.0, -0.49999);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Intersecting));

        pl = planeNormXform(+1.0, +0.0, +0.0, -0.50001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Outside));
        pl = planeNormXform(-1.0, +0.0, +0.0, -0.50001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Outside));
        pl = planeNormXform(+0.0, +1.0, +0.0, -0.50001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Outside));
        pl = planeNormXform(+0.0, -1.0, +0.0, -0.50001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Outside));
        pl = planeNormXform(+0.0, +0.0, +1.0, -0.50001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Outside));
        pl = planeNormXform(+0.0, +0.0, -1.0, -0.50001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Outside));

        // Tests against edges

        pl = planeNormXform(+1.0, +1.0, +0.0, SQRT1_2 + 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Inside));
        pl = planeNormXform(+1.0, -1.0, +0.0, SQRT1_2 + 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Inside));
        pl = planeNormXform(-1.0, +1.0, +0.0, SQRT1_2 + 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Inside));
        pl = planeNormXform(-1.0, -1.0, +0.0, SQRT1_2 + 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Inside));
        pl = planeNormXform(+1.0, +0.0, +1.0, SQRT1_2 + 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Inside));
        pl = planeNormXform(+1.0, +0.0, -1.0, SQRT1_2 + 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Inside));
        pl = planeNormXform(-1.0, +0.0, +1.0, SQRT1_2 + 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Inside));
        pl = planeNormXform(-1.0, +0.0, -1.0, SQRT1_2 + 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Inside));
        pl = planeNormXform(+0.0, +1.0, +1.0, SQRT1_2 + 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Inside));
        pl = planeNormXform(+0.0, +1.0, -1.0, SQRT1_2 + 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Inside));
        pl = planeNormXform(+0.0, -1.0, +1.0, SQRT1_2 + 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Inside));
        pl = planeNormXform(+0.0, -1.0, -1.0, SQRT1_2 + 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Inside));

        pl = planeNormXform(+1.0, +1.0, +0.0, SQRT1_2 - 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Intersecting));
        pl = planeNormXform(+1.0, -1.0, +0.0, SQRT1_2 - 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Intersecting));
        pl = planeNormXform(-1.0, +1.0, +0.0, SQRT1_2 - 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Intersecting));
        pl = planeNormXform(-1.0, -1.0, +0.0, SQRT1_2 - 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Intersecting));
        pl = planeNormXform(+1.0, +0.0, +1.0, SQRT1_2 - 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Intersecting));
        pl = planeNormXform(+1.0, +0.0, -1.0, SQRT1_2 - 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Intersecting));
        pl = planeNormXform(-1.0, +0.0, +1.0, SQRT1_2 - 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Intersecting));
        pl = planeNormXform(-1.0, +0.0, -1.0, SQRT1_2 - 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Intersecting));
        pl = planeNormXform(+0.0, +1.0, +1.0, SQRT1_2 - 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Intersecting));
        pl = planeNormXform(+0.0, +1.0, -1.0, SQRT1_2 - 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Intersecting));
        pl = planeNormXform(+0.0, -1.0, +1.0, SQRT1_2 - 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Intersecting));
        pl = planeNormXform(+0.0, -1.0, -1.0, SQRT1_2 - 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Intersecting));

        pl = planeNormXform(+1.0, +1.0, +0.0, -SQRT1_2 + 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Intersecting));
        pl = planeNormXform(+1.0, -1.0, +0.0, -SQRT1_2 + 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Intersecting));
        pl = planeNormXform(-1.0, +1.0, +0.0, -SQRT1_2 + 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Intersecting));
        pl = planeNormXform(-1.0, -1.0, +0.0, -SQRT1_2 + 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Intersecting));
        pl = planeNormXform(+1.0, +0.0, +1.0, -SQRT1_2 + 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Intersecting));
        pl = planeNormXform(+1.0, +0.0, -1.0, -SQRT1_2 + 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Intersecting));
        pl = planeNormXform(-1.0, +0.0, +1.0, -SQRT1_2 + 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Intersecting));
        pl = planeNormXform(-1.0, +0.0, -1.0, -SQRT1_2 + 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Intersecting));
        pl = planeNormXform(+0.0, +1.0, +1.0, -SQRT1_2 + 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Intersecting));
        pl = planeNormXform(+0.0, +1.0, -1.0, -SQRT1_2 + 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Intersecting));
        pl = planeNormXform(+0.0, -1.0, +1.0, -SQRT1_2 + 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Intersecting));
        pl = planeNormXform(+0.0, -1.0, -1.0, -SQRT1_2 + 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Intersecting));

        pl = planeNormXform(+1.0, +1.0, +0.0, -SQRT1_2 - 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Outside));
        pl = planeNormXform(+1.0, -1.0, +0.0, -SQRT1_2 - 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Outside));
        pl = planeNormXform(-1.0, +1.0, +0.0, -SQRT1_2 - 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Outside));
        pl = planeNormXform(-1.0, -1.0, +0.0, -SQRT1_2 - 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Outside));
        pl = planeNormXform(+1.0, +0.0, +1.0, -SQRT1_2 - 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Outside));
        pl = planeNormXform(+1.0, +0.0, -1.0, -SQRT1_2 - 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Outside));
        pl = planeNormXform(-1.0, +0.0, +1.0, -SQRT1_2 - 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Outside));
        pl = planeNormXform(-1.0, +0.0, -1.0, -SQRT1_2 - 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Outside));
        pl = planeNormXform(+0.0, +1.0, +1.0, -SQRT1_2 - 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Outside));
        pl = planeNormXform(+0.0, +1.0, -1.0, -SQRT1_2 - 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Outside));
        pl = planeNormXform(+0.0, -1.0, +1.0, -SQRT1_2 - 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Outside));
        pl = planeNormXform(+0.0, -1.0, -1.0, -SQRT1_2 - 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Outside));

        // Tests against corners

        pl = planeNormXform(+1.0, +1.0, +1.0, SQRT3_4 + 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Inside));
        pl = planeNormXform(+1.0, +1.0, -1.0, SQRT3_4 + 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Inside));
        pl = planeNormXform(+1.0, -1.0, +1.0, SQRT3_4 + 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Inside));
        pl = planeNormXform(+1.0, -1.0, -1.0, SQRT3_4 + 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Inside));
        pl = planeNormXform(-1.0, +1.0, +1.0, SQRT3_4 + 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Inside));
        pl = planeNormXform(-1.0, +1.0, -1.0, SQRT3_4 + 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Inside));
        pl = planeNormXform(-1.0, -1.0, +1.0, SQRT3_4 + 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Inside));
        pl = planeNormXform(-1.0, -1.0, -1.0, SQRT3_4 + 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Inside));

        pl = planeNormXform(+1.0, +1.0, +1.0, SQRT3_4 - 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Intersecting));
        pl = planeNormXform(+1.0, +1.0, -1.0, SQRT3_4 - 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Intersecting));
        pl = planeNormXform(+1.0, -1.0, +1.0, SQRT3_4 - 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Intersecting));
        pl = planeNormXform(+1.0, -1.0, -1.0, SQRT3_4 - 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Intersecting));
        pl = planeNormXform(-1.0, +1.0, +1.0, SQRT3_4 - 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Intersecting));
        pl = planeNormXform(-1.0, +1.0, -1.0, SQRT3_4 - 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Intersecting));
        pl = planeNormXform(-1.0, -1.0, +1.0, SQRT3_4 - 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Intersecting));
        pl = planeNormXform(-1.0, -1.0, -1.0, SQRT3_4 - 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Intersecting));

        pl = planeNormXform(+1.0, +1.0, +1.0, -SQRT3_4 + 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Intersecting));
        pl = planeNormXform(+1.0, +1.0, -1.0, -SQRT3_4 + 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Intersecting));
        pl = planeNormXform(+1.0, -1.0, +1.0, -SQRT3_4 + 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Intersecting));
        pl = planeNormXform(+1.0, -1.0, -1.0, -SQRT3_4 + 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Intersecting));
        pl = planeNormXform(-1.0, +1.0, +1.0, -SQRT3_4 + 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Intersecting));
        pl = planeNormXform(-1.0, +1.0, -1.0, -SQRT3_4 + 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Intersecting));
        pl = planeNormXform(-1.0, -1.0, +1.0, -SQRT3_4 + 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Intersecting));
        pl = planeNormXform(-1.0, -1.0, -1.0, -SQRT3_4 + 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Intersecting));

        pl = planeNormXform(+1.0, +1.0, +1.0, -SQRT3_4 - 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Outside));
        pl = planeNormXform(+1.0, +1.0, -1.0, -SQRT3_4 - 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Outside));
        pl = planeNormXform(+1.0, -1.0, +1.0, -SQRT3_4 - 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Outside));
        pl = planeNormXform(+1.0, -1.0, -1.0, -SQRT3_4 - 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Outside));
        pl = planeNormXform(-1.0, +1.0, +1.0, -SQRT3_4 - 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Outside));
        pl = planeNormXform(-1.0, +1.0, -1.0, -SQRT3_4 - 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Outside));
        pl = planeNormXform(-1.0, -1.0, +1.0, -SQRT3_4 - 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Outside));
        pl = planeNormXform(-1.0, -1.0, -1.0, -SQRT3_4 - 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == Cesium3DTiles::CullingResult::Outside));
    }
}

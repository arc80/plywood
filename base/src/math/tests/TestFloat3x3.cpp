/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-math/Base.h>
#include <ply-test/TestSuite.h>

namespace ply {

#define PLY_TEST_CASE_PREFIX Float3x3_

namespace {
bool isAlmostEqual(const Float3x3& m1, const Float3x3& m2) {
    return isNear(m1[0], m2[0], 1e-6f) && isNear(m1[1], m2[1], 1e-6f) &&
           isNear(m1[2], m2[2], 1e-6f);
}
} // namespace

PLY_TEST_CASE("Float3x3 constructor") {
    Float3x3 m = {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}};
    PLY_TEST_CHECK(m[0].x == 1 && m[0].y == 2 && m[0].z == 3);
    PLY_TEST_CHECK(m[1].x == 4 && m[1].y == 5 && m[1].z == 6);
    PLY_TEST_CHECK(m[2].x == 7 && m[2].y == 8 && m[2].z == 9);
}

PLY_TEST_CASE("Float3x3 element modification") {
    Float3x3 m = {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}};

    m[0].x = 5;
    m[0].y = 6;
    m[0].z = 7;
    m[1].x = 9;
    m[1].y = 10;
    m[1].z = 15;
    m[2].x = 20;
    m[2].y = 22;
    m[2].z = 24;

    PLY_TEST_CHECK(m[0].x == 5 && m[0].y == 6 && m[0].z == 7);
    PLY_TEST_CHECK(m[1].x == 9 && m[1].y == 10 && m[1].z == 15);
    PLY_TEST_CHECK(m[2].x == 20 && m[2].y == 22 && m[2].z == 24);
}

PLY_TEST_CASE("Float3x3 comparisons") {
    PLY_TEST_CHECK(Float3x3{{2, 3, 5}, {4, 5, 6}, {7, 8, 9}} ==
                   Float3x3{{2, 3, 5}, {4, 5, 6}, {7, 8, 9}});
    PLY_TEST_CHECK(Float3x3{{0, 0, 0}, {0, 0, 0}, {0, 0, 0}} ==
                   Float3x3{{0, -0.f, 0}, {-0.f, 0, 0}, {0, 0, -0.f}});
    PLY_TEST_CHECK(Float3x3{{5, 4, 2}, {3, 2, 1}, {2, 1, 0}} !=
                   Float3x3{{2, 3, 5}, {4, 5, 6}, {7, 8, 9}});

    PLY_TEST_CHECK(
        !(Float3x3{{2, 3, 5}, {4, 5, 6}, {7, 8, 9}} != Float3x3{{2, 3, 5}, {4, 5, 6}, {7, 8, 9}}));
    PLY_TEST_CHECK(!(Float3x3{{0, 0, 0}, {0, 0, 0}, {0, 0, 0}} !=
                     Float3x3{{0, -0.f, 0}, {-0.f, 0, 0}, {0, 0, -0.f}}));
    PLY_TEST_CHECK(
        !(Float3x3{{5, 4, 2}, {3, 2, 1}, {2, 1, 0}} == Float3x3{{2, 3, 5}, {4, 5, 6}, {7, 8, 9}}));
}

PLY_TEST_CASE("Float3x3 creation functions") {
    PLY_TEST_CHECK(Float3x3{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}} == Float3x3::identity());
    PLY_TEST_CHECK(Float3x3{{2, 0, 0}, {0, 2, 0}, {0, 0, 2}} == Float3x3::makeScale(2.f));
}

PLY_TEST_CASE("Float3x3 creation functions from rotation") {
    PLY_TEST_CHECK(isAlmostEqual(Float3x3{{1.f, 0, 0}, {0, 1.f, 0}, {0, 0, 1.f}},
                                 Float3x3::makeRotation({1., 0, 0}, 0)));
    PLY_TEST_CHECK(isAlmostEqual(Float3x3{{1.f, 0, 0}, {0, 1.f, 0}, {0, 0, 1.f}},
                                 Float3x3::makeRotation({0, 1., 0}, 0)));
    PLY_TEST_CHECK(isAlmostEqual(Float3x3{{1.f, 0, 0}, {0, 1.f, 0}, {0, 0, 1.f}},
                                 Float3x3::makeRotation({0, 0, 1.}, 0)));

    PLY_TEST_CHECK(isAlmostEqual(Float3x3{{1.f, 0, 0}, {0, -1.f, 0}, {0, 0, -1.f}},
                                 Float3x3::makeRotation({1., 0, 0}, Pi)));
    PLY_TEST_CHECK(isAlmostEqual(Float3x3{{-1.f, 0, 0}, {0, 1.f, 0}, {0, 0, -1.f}},
                                 Float3x3::makeRotation({0, 1., 0}, Pi)));
    PLY_TEST_CHECK(isAlmostEqual(Float3x3{{-1.f, 0, 0}, {0, -1.f, 0}, {0, 0, 1.f}},
                                 Float3x3::makeRotation({0, 0, 1.}, Pi)));
}

PLY_TEST_CASE("Float3x3 creation functions from quaternion") {
    PLY_TEST_CHECK(isAlmostEqual(Float3x3{{1.f, 0, 0}, {0, 1.f, 0}, {0, 0, 1.f}},
                                 Float3x3::fromQuaternion(Quaternion(Float3{0, 0, 0}, 1))));
    PLY_TEST_CHECK(isAlmostEqual(Float3x3{{1.f, 0, 0}, {0, -1.f, 0}, {0, 0, -1.f}},
                                 Float3x3::fromQuaternion(Quaternion(Float3{1., 0, 0}, 0))));
    PLY_TEST_CHECK(isAlmostEqual(Float3x3{{-1.f, 0, 0}, {0, 1.f, 0}, {0, 0, -1.f}},
                                 Float3x3::fromQuaternion(Quaternion(Float3{0, 1., 0}, 0))));
    PLY_TEST_CHECK(isAlmostEqual(Float3x3{{-1.f, 0, 0}, {0, -1.f, 0}, {0, 0, 1.f}},
                                 Float3x3::fromQuaternion(Quaternion(Float3{0, 0, 1.}, 0))));
}

PLY_TEST_CASE("Float3x3 transposition") {
    PLY_TEST_CHECK(Float3x3{{1, 2, 3}, {4, 5, 6}, {7, 8, 9}} ==
                   Float3x3{{1, 4, 7}, {2, 5, 8}, {3, 6, 9}}.transposed());
    PLY_TEST_CHECK(Float3x3{{-1, 2, -3}, {4, 5, 6}, {7, -8, 9}} ==
                   Float3x3{{-1, 4, 7}, {2, 5, -8}, {-3, 6, 9}}.transposed());
}

PLY_TEST_CASE("Float3x3 transforms a vector") {
    Float3 v1 = {1, 2, 3};
    PLY_TEST_CHECK(isNear(Float3{1, 2, 3}, Float3x3({1, 0, 0}, {0, 1, 0}, {0, 0, 1}) * v1, 0));
    PLY_TEST_CHECK(isNear(Float3{2, 1, 3}, Float3x3({0, 1, 0}, {1, 0, 0}, {0, 0, 1}) * v1, 0));
    PLY_TEST_CHECK(isNear(Float3{1, 3, 2}, Float3x3({1, 0, 0}, {0, 0, 1}, {0, 1, 0}) * v1, 0));

    PLY_TEST_CHECK(isNear(Float3{-3, -2, 5}, Float3x3({0, 0, 5}, {0, -1, 0}, {-1, 0, 0}) * v1, 0));
}

PLY_TEST_CASE("Float3x3 matrix multiplication") {
    Float3x3 m = {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}};

    Float3x3 m1 = Float3x3({1, 0, 0}, {0, 1, 0}, {0, 0, 1})  * m;
    PLY_TEST_CHECK(isAlmostEqual(m, m1));

    Float3x3 m2 = Float3x3({0, 1, 0}, {1, 0, 0}, {0, 0, 1})  * m;
    PLY_TEST_CHECK(isAlmostEqual(Float3x3({2, 1, 3}, {5, 4, 6}, {8, 7, 9}), m2));

    Float3x3 m3 = Float3x3({0, 0, 5}, {0, -1, 0}, {-1, 0, 0})  * m;
    PLY_TEST_CHECK(isAlmostEqual(Float3x3({-3, -2, 5}, {-6, -5, 20}, {-9, -8, 35}), m3));
}

} // namespace ply

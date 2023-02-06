﻿/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-math/Base.h>
#include <ply-test/TestSuite.h>

namespace ply {

#define PLY_TEST_CASE_PREFIX Float3x4_

namespace {
bool is_almost_equal(const Float3x4& m1, const Float3x4& m2) {
    return is_near(m1[0], m2[0], 1e-6f) && is_near(m1[1], m2[1], 1e-6f) &&
           is_near(m1[2], m2[2], 1e-6f) && is_near(m1[3], m2[3], 1e-6f);
}
} // namespace

PLY_TEST_CASE("Float3x4 constructor") {
    Float3x4 m = {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}, {10, 11, 12}};
    PLY_TEST_CHECK(m[0].x == 1 && m[0].y == 2 && m[0].z == 3);
    PLY_TEST_CHECK(m[1].x == 4 && m[1].y == 5 && m[1].z == 6);
    PLY_TEST_CHECK(m[2].x == 7 && m[2].y == 8 && m[2].z == 9);
    PLY_TEST_CHECK(m[3].x == 10 && m[3].y == 11 && m[3].z == 12);
}

PLY_TEST_CASE("Float3x4 conversion as Float3x3") {
    Float3x3 m33 = {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}};
    Float3x4 m = Float3x4{m33, {13, 14, 15}};

    PLY_TEST_CHECK(m33 == m.as_float3x3());
}

PLY_TEST_CASE("Float3x4 element modification") {
    Float3x4 m = {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}, {10, 11, 12}};

    m[0].x = 5;
    m[0].y = 6;
    m[0].z = 7;
    m[1].x = 9;
    m[1].y = 10;
    m[1].z = 15;
    m[2].x = 20;
    m[2].y = 22;
    m[2].z = 24;
    m[3].x = 100;
    m[3].y = 101;
    m[3].z = 102;

    PLY_TEST_CHECK(m[0].x == 5 && m[0].y == 6 && m[0].z == 7);
    PLY_TEST_CHECK(m[1].x == 9 && m[1].y == 10 && m[1].z == 15);
    PLY_TEST_CHECK(m[2].x == 20 && m[2].y == 22 && m[2].z == 24);
    PLY_TEST_CHECK(m[3].x == 100 && m[3].y == 101 && m[3].z == 102);
}

PLY_TEST_CASE("Float3x4 comparisons") {
    PLY_TEST_CHECK(Float3x4{{2, 3, 5}, {4, 5, 6}, {7, 8, 9}, {17, 18, 19}} ==
                   Float3x4{{2, 3, 5}, {4, 5, 6}, {7, 8, 9}, {17, 18, 19}});
    PLY_TEST_CHECK(Float3x4{{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}} ==
                   Float3x4{{-0.f, 0, 0}, {0, 0, -0.f}, {0, -0.f, 0}, {0, 0, 0.f}});
    PLY_TEST_CHECK(Float3x4{{5, 4, 2}, {3, 2, 1}, {2, 1, 0}, {3, 6, 10}} !=
                   Float3x4{{2, 3, 5}, {4, 5, 6}, {7, 8, 9}, {17, 18, 19}});

    PLY_TEST_CHECK(!(Float3x4{{2, 3, 5}, {4, 5, 6}, {7, 8, 9}, {17, 18, 19}} !=
                     Float3x4{{2, 3, 5}, {4, 5, 6}, {7, 8, 9}, {17, 18, 19}}));
    PLY_TEST_CHECK(!(Float3x4{{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}} !=
                     Float3x4{{-0.f, 0, 0}, {0, 0, -0.f}, {0, -0.f, 0}, {0, 0, 0.f}}));
    PLY_TEST_CHECK(!(Float3x4{{5, 4, 2}, {3, 2, 1}, {2, 1, 0}, {3, 6, 10}} ==
                     Float3x4{{2, 3, 5}, {4, 5, 6}, {7, 8, 9}, {17, 18, 19}}));
}

PLY_TEST_CASE("Float3x4 creation functions") {
    PLY_TEST_CHECK(Float3x4{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {0, 0, 0}} ==
                   Float3x4::identity());
    PLY_TEST_CHECK(Float3x4{{2, 0, 0}, {0, 2, 0}, {0, 0, 2}, {0, 0, 0}} ==
                   Float3x4::make_scale(2.f));
}

PLY_TEST_CASE("Float3x4 creation functions from rotation") {
    PLY_TEST_CHECK(
        is_almost_equal(Float3x4{{1.f, 0, 0}, {0, 1.f, 0}, {0, 0, 1.f}, {0, 0, 0}},
                        Float3x4::make_rotation({1., 0, 0}, 0)));
    PLY_TEST_CHECK(
        is_almost_equal(Float3x4{{1.f, 0, 0}, {0, 1.f, 0}, {0, 0, 1.f}, {0, 0, 0}},
                        Float3x4::make_rotation({0, 1., 0}, 0)));
    PLY_TEST_CHECK(
        is_almost_equal(Float3x4{{1.f, 0, 0}, {0, 1.f, 0}, {0, 0, 1.f}, {0, 0, 0}},
                        Float3x4::make_rotation({0, 0, 1.}, 0)));

    PLY_TEST_CHECK(
        is_almost_equal(Float3x4{{1.f, 0, 0}, {0, -1.f, 0}, {0, 0, -1.f}, {0, 0, 0}},
                        Float3x4::make_rotation({1., 0, 0}, Pi)));
    PLY_TEST_CHECK(
        is_almost_equal(Float3x4{{-1.f, 0, 0}, {0, 1.f, 0}, {0, 0, -1.f}, {0, 0, 0}},
                        Float3x4::make_rotation({0, 1., 0}, Pi)));
    PLY_TEST_CHECK(
        is_almost_equal(Float3x4{{-1.f, 0, 0}, {0, -1.f, 0}, {0, 0, 1.f}, {0, 0, 0}},
                        Float3x4::make_rotation({0, 0, 1.}, Pi)));
}

PLY_TEST_CASE("Float3x4 creation functions from translation") {
    PLY_TEST_CHECK(
        is_almost_equal(Float3x4{{1.f, 0, 0}, {0, 1.f, 0}, {0, 0, 1.f}, {1, 0, 0}},
                        Float3x4::make_translation({1., 0, 0})));
    PLY_TEST_CHECK(
        is_almost_equal(Float3x4{{1.f, 0, 0}, {0, 1.f, 0}, {0, 0, 1.f}, {0, 1, 0}},
                        Float3x4::make_translation({0, 1., 0})));
    PLY_TEST_CHECK(
        is_almost_equal(Float3x4{{1.f, 0, 0}, {0, 1.f, 0}, {0, 0, 1.f}, {0, 0, 1}},
                        Float3x4::make_translation({0, 0, 1.})));

    PLY_TEST_CHECK(
        is_almost_equal(Float3x4{{1.f, 0, 0}, {0, 1.f, 0}, {0, 0, 1.f}, {-1, 2, 0}},
                        Float3x4::make_translation({-1., 2, -0})));
}

PLY_TEST_CASE("Float3x4 creation functions from quaternion") {
    PLY_TEST_CHECK(is_almost_equal(
        Float3x4{{1.f, 0, 0}, {0, 1.f, 0}, {0, 0, 1.f}, {1, 2, 3}},
        Float3x4::from_quaternion(Quaternion(Float3{0, 0, 0}, 1), Float3{1, 2, 3})));
    PLY_TEST_CHECK(is_almost_equal(
        Float3x4{{1.f, 0, 0}, {0, -1.f, 0}, {0, 0, -1.f}, {-1, 0, 0}},
        Float3x4::from_quaternion(Quaternion(Float3{1., 0, 0}, 0), Float3{-1, -0, 0})));
    PLY_TEST_CHECK(is_almost_equal(
        Float3x4{{-1.f, 0, 0}, {0, 1.f, 0}, {0, 0, -1.f}, {3, 2, 1}},
        Float3x4::from_quaternion(Quaternion(Float3{0, 1., 0}, 0), Float3{3, 2, 1})));
    PLY_TEST_CHECK(
        is_almost_equal(Float3x4{{-1.f, 0, 0}, {0, -1.f, 0}, {0, 0, 1.f}, {-4, -5, -6}},
                        Float3x4::from_quaternion(Quaternion(Float3{0, 0, 1.}, 0),
                                                  Float3{-4, -5, -6})));
}

PLY_TEST_CASE("Float3x4 creation functions from quatpos") {
    PLY_TEST_CHECK(
        is_almost_equal(Float3x4{{1.f, 0, 0}, {0, 1.f, 0}, {0, 0, 1.f}, {1, 2, 3}},
                        Float3x4::from_quat_pos(
                            QuatPos(Quaternion(Float3{0, 0, 0}, 1), Float3{1, 2, 3}))));
    PLY_TEST_CHECK(
        is_almost_equal(Float3x4{{1.f, 0, 0}, {0, -1.f, 0}, {0, 0, -1.f}, {-1, 0, 0}},
                        Float3x4::from_quat_pos(QuatPos(Quaternion(Float3{1., 0, 0}, 0),
                                                        Float3{-1, -0, 0}))));
    PLY_TEST_CHECK(
        is_almost_equal(Float3x4{{-1.f, 0, 0}, {0, 1.f, 0}, {0, 0, -1.f}, {3, 2, 1}},
                        Float3x4::from_quat_pos(QuatPos(Quaternion(Float3{0, 1., 0}, 0),
                                                        Float3{3, 2, 1}))));
    PLY_TEST_CHECK(
        is_almost_equal(Float3x4{{-1.f, 0, 0}, {0, -1.f, 0}, {0, 0, 1.f}, {-4, -5, -6}},
                        Float3x4::from_quat_pos(QuatPos(Quaternion(Float3{0, 0, 1.}, 0),
                                                        Float3{-4, -5, -6}))));
}

PLY_TEST_CASE("Float3x4 inversion") {
    PLY_TEST_CHECK(
        Float3x4{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {0, 0, 0}} ==
        Float3x4{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {0, 0, 0}}.inverted_ortho());
    PLY_TEST_CHECK(
        Float3x4{{0, 0, 1}, {1, 0, 0}, {0, 1, 0}, {-2, 0, 0}} ==
        Float3x4{{0, 1, 0}, {0, 0, 1}, {1, 0, 0}, {0, 2, 0}}.inverted_ortho());
}

PLY_TEST_CASE("Float3x4 transforms a Float3 vector") {
    Float3 v1 = {1, 2, 3};
    PLY_TEST_CHECK(is_near(
        Float3{1, 2, 3}, Float3x4({1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {0, 0, 0}) * v1, 0));
    PLY_TEST_CHECK(is_near(
        Float3{2, 1, 3}, Float3x4({0, 1, 0}, {1, 0, 0}, {0, 0, 1}, {0, 0, 0}) * v1, 0));
    PLY_TEST_CHECK(is_near(
        Float3{1, 3, 2}, Float3x4({1, 0, 0}, {0, 0, 1}, {0, 1, 0}, {0, 0, 0}) * v1, 0));

    PLY_TEST_CHECK(is_near(Float3{-3, -2, 5},
                           Float3x4({0, 0, 5}, {0, -1, 0}, {-1, 0, 0}, {0, 0, 0}) * v1,
                           0));
}

PLY_TEST_CASE("Float3x4 transforms a Float4 vector") {
    Float4 v1 = {1, 2, 3, 1};
    PLY_TEST_CHECK(is_near(Float4{1, 2, 3, 1},
                           Float3x4({1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {0, 0, 0}) * v1,
                           0));
    PLY_TEST_CHECK(is_near(Float4{2, 1, 3, 1},
                           Float3x4({0, 1, 0}, {1, 0, 0}, {0, 0, 1}, {0, 0, 0}) * v1,
                           0));
    PLY_TEST_CHECK(is_near(Float4{1, 3, 2, 1},
                           Float3x4({1, 0, 0}, {0, 0, 1}, {0, 1, 0}, {0, 0, 0}) * v1,
                           0));

    PLY_TEST_CHECK(is_near(Float4{-3, -2, 5, 1},
                           Float3x4({0, 0, 5}, {0, -1, 0}, {-1, 0, 0}, {0, 0, 0}) * v1,
                           0));

    PLY_TEST_CHECK(is_near(Float4{2, 4, 6, 1},
                           Float3x4({1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {1, 2, 3}) * v1,
                           0));
    PLY_TEST_CHECK(is_near(Float4{3, 3, 6, 1},
                           Float3x4({0, 1, 0}, {1, 0, 0}, {0, 0, 1}, {1, 2, 3}) * v1,
                           0));
    PLY_TEST_CHECK(is_near(Float4{2, 5, 5, 1},
                           Float3x4({1, 0, 0}, {0, 0, 1}, {0, 1, 0}, {1, 2, 3}) * v1,
                           0));
}

PLY_TEST_CASE("Float3x4 matrix multiplication") {
    Float3x4 m = {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}, {1, 0, 0}};

    Float3x4 m1 = Float3x4({1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {1, 0, 0}) * m;
    PLY_TEST_CHECK(
        is_almost_equal(Float3x4{{1, 2, 3}, {4, 5, 6}, {7, 8, 9}, {2, 0, 0}}, m1));

    Float3x4 m2 = Float3x4({0, 1, 0}, {1, 0, 0}, {0, 0, 1}, {0, 0, 0}) * m;
    PLY_TEST_CHECK(
        is_almost_equal(Float3x4({2, 1, 3}, {5, 4, 6}, {8, 7, 9}, {0, 1, 0}), m2));

    Float3x4 m3 = Float3x4({0, 0, 5}, {0, -1, 0}, {-1, 0, 0}, {0, 0, 0}) * m;
    PLY_TEST_CHECK(is_almost_equal(
        Float3x4({-3, -2, 5}, {-6, -5, 20}, {-9, -8, 35}, {0, 0, 5}), m3));
}

} // namespace ply

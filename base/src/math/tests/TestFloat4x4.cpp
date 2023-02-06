/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-math/Base.h>
#include <ply-test/TestSuite.h>

namespace ply {

#define PLY_TEST_CASE_PREFIX Float4x4_

namespace {
bool is_almost_equal(const Float4x4& m1, const Float4x4& m2) {
    return is_near(m1[0], m2[0], 1e-6f) && is_near(m1[1], m2[1], 1e-6f) &&
           is_near(m1[2], m2[2], 1e-6f) && is_near(m1[3], m2[3], 1e-6f);
}
} // namespace

PLY_TEST_CASE("Float4x4 constructor") {
    Float4x4 m = {{1, 2, 3, 13}, {4, 5, 6, 14}, {7, 8, 9, 15}, {10, 11, 12, 16}};
    PLY_TEST_CHECK(m[0].x == 1 && m[0].y == 2 && m[0].z == 3 && m[0].w == 13);
    PLY_TEST_CHECK(m[1].x == 4 && m[1].y == 5 && m[1].z == 6 && m[1].w == 14);
    PLY_TEST_CHECK(m[2].x == 7 && m[2].y == 8 && m[2].z == 9 && m[2].w == 15);
    PLY_TEST_CHECK(m[3].x == 10 && m[3].y == 11 && m[3].z == 12 && m[3].w == 16);

    Float4x4 m2 = Float4x4{{{1, 2, 3}, {4, 5, 6}, {7, 8, 9}}, {13, 14, 15}};
    PLY_TEST_CHECK(m2[0].x == 1 && m2[0].y == 2 && m2[0].z == 3 && m2[0].w == 0);
    PLY_TEST_CHECK(m2[1].x == 4 && m2[1].y == 5 && m2[1].z == 6 && m2[1].w == 0);
    PLY_TEST_CHECK(m2[2].x == 7 && m2[2].y == 8 && m2[2].z == 9 && m2[2].w == 0);
    PLY_TEST_CHECK(m2[3].x == 13 && m2[3].y == 14 && m2[3].z == 15 && m2[3].w == 1);
}

PLY_TEST_CASE("Float4x4 conversion to Float3x3") {
    Float3x3 m33 = {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}};
    Float4x4 m = Float4x4{m33, {13, 14, 15}};

    PLY_TEST_CHECK(m33 == m.to_float3x3());
}

PLY_TEST_CASE("Float4x4 conversion to Float3x4") {
    Float3x3 m33 = {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}};
    Float4x4 m = Float4x4{m33, {13, 14, 15}};

    PLY_TEST_CHECK(m33 == m.to_float3x3());
}

PLY_TEST_CASE("Float4x4 element modification") {
    Float4x4 m = {{1, 2, 3, 13}, {4, 5, 6, 14}, {7, 8, 9, 15}, {10, 11, 12, 16}};

    m[0].x = 5;
    m[0].y = 6;
    m[0].z = 7;
    m[0].w = -1;
    m[1].x = 9;
    m[1].y = 10;
    m[1].z = 15;
    m[1].w = -2;
    m[2].x = 20;
    m[2].y = 22;
    m[2].z = 24;
    m[2].w = -3;
    m[3].x = 100;
    m[3].y = 101;
    m[3].z = 102;
    m[3].w = -4;

    PLY_TEST_CHECK(m[0].x == 5 && m[0].y == 6 && m[0].z == 7 && m[0].w == -1);
    PLY_TEST_CHECK(m[1].x == 9 && m[1].y == 10 && m[1].z == 15 && m[1].w == -2);
    PLY_TEST_CHECK(m[2].x == 20 && m[2].y == 22 && m[2].z == 24 && m[2].w == -3);
    PLY_TEST_CHECK(m[3].x == 100 && m[3].y == 101 && m[3].z == 102 && m[3].w == -4);
}

PLY_TEST_CASE("Float4x4 comparisons") {
    PLY_TEST_CHECK(
        Float4x4{{2, 3, 5, 20}, {4, 5, 6, 21}, {7, 8, 9, 22}, {17, 18, 19, 23}} ==
        Float4x4{{2, 3, 5, 20}, {4, 5, 6, 21}, {7, 8, 9, 22}, {17, 18, 19, 23}});
    PLY_TEST_CHECK(
        Float4x4{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}} ==
        Float4x4{{-0.f, 0, 0, -0}, {0, 0, -0.f, 0}, {0, -0.f, 0, 0}, {0, 0, 0.f, -0}});
    PLY_TEST_CHECK(
        Float4x4{{5, 4, 2, 2}, {3, 2, 1, 7}, {2, 1, 0, 0}, {3, 6, 10, 5}} !=
        Float4x4{{2, 3, 5, 1}, {4, 5, 6, -2}, {7, 8, 9, 0}, {17, 18, 19, -6}});

    PLY_TEST_CHECK(
        !(Float4x4{{2, 3, 5, 20}, {4, 5, 6, 21}, {7, 8, 9, 22}, {17, 18, 19, 23}} !=
          Float4x4{{2, 3, 5, 20}, {4, 5, 6, 21}, {7, 8, 9, 22}, {17, 18, 19, 23}}));
    PLY_TEST_CHECK(!(
        Float4x4{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}} !=
        Float4x4{{-0.f, 0, 0, -0}, {0, 0, -0.f, 0}, {0, -0.f, 0, 0}, {0, 0, 0.f, -0}}));
    PLY_TEST_CHECK(
        !(Float4x4{{5, 4, 2, 2}, {3, 2, 1, 7}, {2, 1, 0, 0}, {3, 6, 10, 5}} ==
          Float4x4{{2, 3, 5, 1}, {4, 5, 6, -2}, {7, 8, 9, 0}, {17, 18, 19, -6}}));
}

PLY_TEST_CASE("Float4x4 creation functions") {
    PLY_TEST_CHECK(Float4x4{{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}} ==
                   Float4x4::identity());
    PLY_TEST_CHECK(Float4x4{{2, 0, 0, 0}, {0, 2, 0, 0}, {0, 0, 2, 0}, {0, 0, 0, 1}} ==
                   Float4x4::make_scale(2.f));
}

PLY_TEST_CASE("Float4x4 creation functions from rotation") {
    PLY_TEST_CHECK(is_almost_equal(
        Float4x4{{1.f, 0, 0, 0}, {0, 1.f, 0, 0}, {0, 0, 1.f, 0}, {0, 0, 0, 1}},
        Float4x4::make_rotation({1., 0, 0}, 0)));
    PLY_TEST_CHECK(is_almost_equal(
        Float4x4{{1.f, 0, 0, 0}, {0, 1.f, 0, 0}, {0, 0, 1.f, 0}, {0, 0, 0, 1}},
        Float4x4::make_rotation({0, 1., 0}, 0)));
    PLY_TEST_CHECK(is_almost_equal(
        Float4x4{{1.f, 0, 0, 0}, {0, 1.f, 0, 0}, {0, 0, 1.f, 0}, {0, 0, 0, 1}},
        Float4x4::make_rotation({0, 0, 1.}, 0)));

    PLY_TEST_CHECK(is_almost_equal(
        Float4x4{{1.f, 0, 0, 0}, {0, -1.f, 0, 0}, {0, 0, -1.f, 0}, {0, 0, 0, 1}},
        Float4x4::make_rotation({1., 0, 0}, Pi)));
    PLY_TEST_CHECK(is_almost_equal(
        Float4x4{{-1.f, 0, 0, 0}, {0, 1.f, 0, 0}, {0, 0, -1.f, 0}, {0, 0, 0, 1}},
        Float4x4::make_rotation({0, 1., 0}, Pi)));
    PLY_TEST_CHECK(is_almost_equal(
        Float4x4{{-1.f, 0, 0, 0}, {0, -1.f, 0, 0}, {0, 0, 1.f, 0}, {0, 0, 0, 1}},
        Float4x4::make_rotation({0, 0, 1.}, Pi)));
}

PLY_TEST_CASE("Float4x4 creation functions from translation") {
    PLY_TEST_CHECK(is_almost_equal(
        Float4x4{{1.f, 0, 0, 0}, {0, 1.f, 0, 0}, {0, 0, 1.f, 0}, {1, 0, 0, 1}},
        Float4x4::make_translation({1., 0, 0})));
    PLY_TEST_CHECK(is_almost_equal(
        Float4x4{{1.f, 0, 0, 0}, {0, 1.f, 0, 0}, {0, 0, 1.f, 0}, {0, 1, 0, 1}},
        Float4x4::make_translation({0, 1., 0})));
    PLY_TEST_CHECK(is_almost_equal(
        Float4x4{{1.f, 0, 0, 0}, {0, 1.f, 0, 0}, {0, 0, 1.f, 0}, {0, 0, 1, 1}},
        Float4x4::make_translation({0, 0, 1.})));

    PLY_TEST_CHECK(is_almost_equal(
        Float4x4{{1.f, 0, 0, 0}, {0, 1.f, 0, 0}, {0, 0, 1.f, 0}, {-1, 2, 0, 1}},
        Float4x4::make_translation({-1., 2, -0})));
}

PLY_TEST_CASE("Float4x4 creation functions from quaternion") {
    PLY_TEST_CHECK(is_almost_equal(
        Float4x4{{1.f, 0, 0, 0}, {0, 1.f, 0, 0}, {0, 0, 1.f, 0}, {1, 2, 3, 1}},
        Float4x4::from_quaternion(Quaternion(Float3{0, 0, 0}, 1), Float3{1, 2, 3})));
    PLY_TEST_CHECK(is_almost_equal(
        Float4x4{{1.f, 0, 0, 0}, {0, -1.f, 0, 0}, {0, 0, -1.f, 0}, {-1, 0, 0, 1}},
        Float4x4::from_quaternion(Quaternion(Float3{1., 0, 0}, 0), Float3{-1, -0, 0})));
    PLY_TEST_CHECK(is_almost_equal(
        Float4x4{{-1.f, 0, 0, 0}, {0, 1.f, 0, 0}, {0, 0, -1.f, 0}, {3, 2, 1, 1}},
        Float4x4::from_quaternion(Quaternion(Float3{0, 1., 0}, 0), Float3{3, 2, 1})));
    PLY_TEST_CHECK(is_almost_equal(
        Float4x4{{-1.f, 0, 0, 0}, {0, -1.f, 0, 0}, {0, 0, 1.f, 0}, {-4, -5, -6, 1}},
        Float4x4::from_quaternion(Quaternion(Float3{0, 0, 1.}, 0),
                                  Float3{-4, -5, -6})));
}

PLY_TEST_CASE("Float4x4 creation functions from quatpos") {
    PLY_TEST_CHECK(is_almost_equal(
        Float4x4{{1.f, 0, 0, 0}, {0, 1.f, 0, 0}, {0, 0, 1.f, 0}, {1, 2, 3, 1}},
        Float4x4::from_quat_pos(
            QuatPos(Quaternion(Float3{0, 0, 0}, 1), Float3{1, 2, 3}))));
    PLY_TEST_CHECK(is_almost_equal(
        Float4x4{{1.f, 0, 0, 0}, {0, -1.f, 0, 0}, {0, 0, -1.f, 0}, {-1, 0, 0, 1}},
        Float4x4::from_quat_pos(
            QuatPos(Quaternion(Float3{1., 0, 0}, 0), Float3{-1, -0, 0}))));
    PLY_TEST_CHECK(is_almost_equal(
        Float4x4{{-1.f, 0, 0, 0}, {0, 1.f, 0, 0}, {0, 0, -1.f, 0}, {3, 2, 1, 1}},
        Float4x4::from_quat_pos(
            QuatPos(Quaternion(Float3{0, 1., 0}, 0), Float3{3, 2, 1}))));
    PLY_TEST_CHECK(is_almost_equal(
        Float4x4{{-1.f, 0, 0, 0}, {0, -1.f, 0, 0}, {0, 0, 1.f, 0}, {-4, -5, -6, 1}},
        Float4x4::from_quat_pos(
            QuatPos(Quaternion(Float3{0, 0, 1.}, 0), Float3{-4, -5, -6}))));
}

PLY_TEST_CASE("Float4x4 perspective projection") {
    Float4x4 p = Float4x4::make_projection(rect_from_fov(Pi / 2, 16.f / 9), 1, 100);
    PLY_TEST_CHECK(is_almost_equal(Float4x4{{0.5625f, 0, 0, 0},
                                            {0, 1.f, 0, 0},
                                            {0, 0, -1.02020204f, -1},
                                            {0, 0, -2.02020192f, 0}},
                                   p));
}

PLY_TEST_CASE("Float4x4 orthogonal projection") {
    Float4x4 o = Float4x4::make_ortho(Rect::from_size(-1.f, 1.f, 2.f, 2.f), 1, 100);
    PLY_TEST_CHECK(is_almost_equal(Float4x4{{1, 0, 0, 0},
                                            {0, 1, 0, 0},
                                            {0, 0, -0.0202020202f, 0},
                                            {0, -2, -1.02020204f, 1}},
                                   o));
}

PLY_TEST_CASE("Float4x4 transposition") {
    PLY_TEST_CHECK(
        Float4x4{{1, 2, 3, 4}, {5, 6, 7, 8}, {9, 10, 11, 12}, {13, 14, 15, 16}} ==
        Float4x4{{1, 5, 9, 13}, {2, 6, 10, 14}, {3, 7, 11, 15}, {4, 8, 12, 16}}
            .transposed());
    PLY_TEST_CHECK(
        Float4x4{{-1, 2, 3, 4}, {5, 6, -7, 8}, {9, 10, 11, 12}, {13, -14, 15, 16}} ==
        Float4x4{{-1, 5, 9, 13}, {2, 6, 10, -14}, {3, -7, 11, 15}, {4, 8, 12, 16}}
            .transposed());
}

PLY_TEST_CASE("Float4x4 inversion") {
    PLY_TEST_CHECK(Float4x4{{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}} ==
                   Float4x4{{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}}
                       .inverted_ortho());
    PLY_TEST_CHECK(Float4x4{{0, 0, 1, 0}, {1, 0, 0, 0}, {0, 1, 0, 0}, {-2, 0, 0, 1}} ==
                   Float4x4{{0, 1, 0, 0}, {0, 0, 1, 0}, {1, 0, 0, 0}, {0, 2, 0, 1}}
                       .inverted_ortho());
}

PLY_TEST_CASE("Float4x4 transforms a Float3 vector") {
    Float4 v1 = {1, 2, 3, 1};
    PLY_TEST_CHECK(is_near(
        Float4{1, 2, 3, 1},
        Float4x4({1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}) * v1, 0));
    PLY_TEST_CHECK(is_near(
        Float4{2, 1, 3, 1},
        Float4x4({0, 1, 0, 0}, {1, 0, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}) * v1, 0));
    PLY_TEST_CHECK(is_near(
        Float4{1, 3, 2, 1},
        Float4x4({1, 0, 0, 0}, {0, 0, 1, 0}, {0, 1, 0, 0}, {0, 0, 0, 1}) * v1, 0));

    PLY_TEST_CHECK(is_near(
        Float4{-3, -2, 5, 1},
        Float4x4({0, 0, 5, 0}, {0, -1, 0, 0}, {-1, 0, 0, 0}, {0, 0, 0, 1}) * v1, 0));
}

PLY_TEST_CASE("Float4x4 matrix multiplication") {
    Float4x4 m = {{1, 2, 3, 0}, {4, 5, 6, 0}, {7, 8, 9, 0}, {1, 0, 0, 1}};

    Float4x4 m1 = Float4x4({1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {1, 0, 0, 1}) * m;
    PLY_TEST_CHECK(is_almost_equal(
        Float4x4{{1, 2, 3, 0}, {4, 5, 6, 0}, {7, 8, 9, 0}, {2, 0, 0, 1}}, m1));

    Float4x4 m2 = Float4x4({0, 1, 0, 0}, {1, 0, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}) * m;
    PLY_TEST_CHECK(is_almost_equal(
        Float4x4({2, 1, 3, 0}, {5, 4, 6, 0}, {8, 7, 9, 0}, {0, 1, 0, 1}), m2));

    Float4x4 m3 =
        Float4x4({0, 0, 5, 0}, {0, -1, 0, 0}, {-1, 0, 0, 0}, {0, 0, 0, 1}) * m;
    PLY_TEST_CHECK(is_almost_equal(
        Float4x4({-3, -2, 5, 0}, {-6, -5, 20, 0}, {-9, -8, 35, 0}, {0, 0, 5, 1}), m3));
}

PLY_TEST_CASE("Float4x4 matrix multiplication by Float3x4") {
    Float3x4 m = {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}, {1, 0, 0}};

    Float4x4 m1 = Float4x4({1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {1, 0, 0, 1}) * m;
    PLY_TEST_CHECK(is_almost_equal(
        Float4x4{{1, 2, 3, 0}, {4, 5, 6, 0}, {7, 8, 9, 0}, {2, 0, 0, 1}}, m1));

    Float4x4 m2 = Float4x4({0, 1, 0, 0}, {1, 0, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}) * m;
    PLY_TEST_CHECK(is_almost_equal(
        Float4x4({2, 1, 3, 0}, {5, 4, 6, 0}, {8, 7, 9, 0}, {0, 1, 0, 1}), m2));

    Float4x4 m3 =
        Float4x4({0, 0, 5, 0}, {0, -1, 0, 0}, {-1, 0, 0, 0}, {0, 0, 0, 1}) * m;
    PLY_TEST_CHECK(is_almost_equal(
        Float4x4({-3, -2, 5, 0}, {-6, -5, 20, 0}, {-9, -8, 35, 0}, {0, 0, 5, 1}), m3));
}

PLY_TEST_CASE("Float3x4 matrix multiplication by Float4x4") {
    Float4x4 m = {{1, 2, 3, 0}, {4, 5, 6, 0}, {7, 8, 9, 0}, {1, 0, 0, 1}};

    Float4x4 m1 = Float3x4({1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {1, 0, 0}) * m;
    PLY_TEST_CHECK(is_almost_equal(
        Float4x4{{1, 2, 3, 0}, {4, 5, 6, 0}, {7, 8, 9, 0}, {2, 0, 0, 1}}, m1));

    Float4x4 m2 = Float3x4({0, 1, 0}, {1, 0, 0}, {0, 0, 1}, {0, 0, 0}) * m;
    PLY_TEST_CHECK(is_almost_equal(
        Float4x4({2, 1, 3, 0}, {5, 4, 6, 0}, {8, 7, 9, 0}, {0, 1, 0, 1}), m2));

    Float4x4 m3 = Float3x4({0, 0, 5}, {0, -1, 0}, {-1, 0, 0}, {0, 0, 0}) * m;
    PLY_TEST_CHECK(is_almost_equal(
        Float4x4({-3, -2, 5, 0}, {-6, -5, 20, 0}, {-9, -8, 35, 0}, {0, 0, 5, 1}), m3));
}

} // namespace ply

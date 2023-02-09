/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-math.h>
#include <ply-test/TestSuite.h>

namespace ply {

#define PLY_TEST_CASE_PREFIX Float3x3_

namespace {
bool is_almost_equal(const mat3x3& m1, const mat3x3& m2) {
    return is_within(m1[0], m2[0], 1e-6f) && is_within(m1[1], m2[1], 1e-6f) &&
           is_within(m1[2], m2[2], 1e-6f);
}
} // namespace

PLY_TEST_CASE("mat3x3 constructor") {
    mat3x3 m = {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}};
    PLY_TEST_CHECK(m[0].x == 1 && m[0].y == 2 && m[0].z == 3);
    PLY_TEST_CHECK(m[1].x == 4 && m[1].y == 5 && m[1].z == 6);
    PLY_TEST_CHECK(m[2].x == 7 && m[2].y == 8 && m[2].z == 9);
}

PLY_TEST_CASE("mat3x3 element modification") {
    mat3x3 m = {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}};

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

PLY_TEST_CASE("mat3x3 comparisons") {
    PLY_TEST_CHECK(mat3x3{{2, 3, 5}, {4, 5, 6}, {7, 8, 9}} ==
                   mat3x3{{2, 3, 5}, {4, 5, 6}, {7, 8, 9}});
    PLY_TEST_CHECK(mat3x3{{0, 0, 0}, {0, 0, 0}, {0, 0, 0}} ==
                   mat3x3{{0, -0.f, 0}, {-0.f, 0, 0}, {0, 0, -0.f}});
    PLY_TEST_CHECK(mat3x3{{5, 4, 2}, {3, 2, 1}, {2, 1, 0}} !=
                   mat3x3{{2, 3, 5}, {4, 5, 6}, {7, 8, 9}});

    PLY_TEST_CHECK(!(mat3x3{{2, 3, 5}, {4, 5, 6}, {7, 8, 9}} !=
                     mat3x3{{2, 3, 5}, {4, 5, 6}, {7, 8, 9}}));
    PLY_TEST_CHECK(!(mat3x3{{0, 0, 0}, {0, 0, 0}, {0, 0, 0}} !=
                     mat3x3{{0, -0.f, 0}, {-0.f, 0, 0}, {0, 0, -0.f}}));
    PLY_TEST_CHECK(!(mat3x3{{5, 4, 2}, {3, 2, 1}, {2, 1, 0}} ==
                     mat3x3{{2, 3, 5}, {4, 5, 6}, {7, 8, 9}}));
}

PLY_TEST_CASE("mat3x3 creation functions") {
    PLY_TEST_CHECK(mat3x3{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}} == mat3x3::identity());
    PLY_TEST_CHECK(mat3x3{{2, 0, 0}, {0, 2, 0}, {0, 0, 2}} == mat3x3::make_scale(2.f));
}

PLY_TEST_CASE("mat3x3 creation functions from rotation") {
    PLY_TEST_CHECK(is_almost_equal(mat3x3{{1.f, 0, 0}, {0, 1.f, 0}, {0, 0, 1.f}},
                                   mat3x3::make_rotation({1., 0, 0}, 0)));
    PLY_TEST_CHECK(is_almost_equal(mat3x3{{1.f, 0, 0}, {0, 1.f, 0}, {0, 0, 1.f}},
                                   mat3x3::make_rotation({0, 1., 0}, 0)));
    PLY_TEST_CHECK(is_almost_equal(mat3x3{{1.f, 0, 0}, {0, 1.f, 0}, {0, 0, 1.f}},
                                   mat3x3::make_rotation({0, 0, 1.}, 0)));

    PLY_TEST_CHECK(is_almost_equal(mat3x3{{1.f, 0, 0}, {0, -1.f, 0}, {0, 0, -1.f}},
                                   mat3x3::make_rotation({1., 0, 0}, Pi)));
    PLY_TEST_CHECK(is_almost_equal(mat3x3{{-1.f, 0, 0}, {0, 1.f, 0}, {0, 0, -1.f}},
                                   mat3x3::make_rotation({0, 1., 0}, Pi)));
    PLY_TEST_CHECK(is_almost_equal(mat3x3{{-1.f, 0, 0}, {0, -1.f, 0}, {0, 0, 1.f}},
                                   mat3x3::make_rotation({0, 0, 1.}, Pi)));
}

PLY_TEST_CASE("mat3x3 creation functions from quaternion") {
    PLY_TEST_CHECK(
        is_almost_equal(mat3x3{{1.f, 0, 0}, {0, 1.f, 0}, {0, 0, 1.f}},
                        mat3x3::from_quaternion(Quaternion(vec3{0, 0, 0}, 1))));
    PLY_TEST_CHECK(
        is_almost_equal(mat3x3{{1.f, 0, 0}, {0, -1.f, 0}, {0, 0, -1.f}},
                        mat3x3::from_quaternion(Quaternion(vec3{1., 0, 0}, 0))));
    PLY_TEST_CHECK(
        is_almost_equal(mat3x3{{-1.f, 0, 0}, {0, 1.f, 0}, {0, 0, -1.f}},
                        mat3x3::from_quaternion(Quaternion(vec3{0, 1., 0}, 0))));
    PLY_TEST_CHECK(
        is_almost_equal(mat3x3{{-1.f, 0, 0}, {0, -1.f, 0}, {0, 0, 1.f}},
                        mat3x3::from_quaternion(Quaternion(vec3{0, 0, 1.}, 0))));
}

PLY_TEST_CASE("mat3x3 transposition") {
    PLY_TEST_CHECK(mat3x3{{1, 2, 3}, {4, 5, 6}, {7, 8, 9}} ==
                   mat3x3{{1, 4, 7}, {2, 5, 8}, {3, 6, 9}}.transposed());
    PLY_TEST_CHECK(mat3x3{{-1, 2, -3}, {4, 5, 6}, {7, -8, 9}} ==
                   mat3x3{{-1, 4, 7}, {2, 5, -8}, {-3, 6, 9}}.transposed());
}

PLY_TEST_CASE("mat3x3 transforms a vector") {
    vec3 v1 = {1, 2, 3};
    PLY_TEST_CHECK(
        is_within(vec3{1, 2, 3}, mat3x3({1, 0, 0}, {0, 1, 0}, {0, 0, 1}) * v1, 0));
    PLY_TEST_CHECK(
        is_within(vec3{2, 1, 3}, mat3x3({0, 1, 0}, {1, 0, 0}, {0, 0, 1}) * v1, 0));
    PLY_TEST_CHECK(
        is_within(vec3{1, 3, 2}, mat3x3({1, 0, 0}, {0, 0, 1}, {0, 1, 0}) * v1, 0));

    PLY_TEST_CHECK(
        is_within(vec3{-3, -2, 5}, mat3x3({0, 0, 5}, {0, -1, 0}, {-1, 0, 0}) * v1, 0));
}

PLY_TEST_CASE("mat3x3 matrix multiplication") {
    mat3x3 m = {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}};

    mat3x3 m1 = mat3x3({1, 0, 0}, {0, 1, 0}, {0, 0, 1}) * m;
    PLY_TEST_CHECK(is_almost_equal(m, m1));

    mat3x3 m2 = mat3x3({0, 1, 0}, {1, 0, 0}, {0, 0, 1}) * m;
    PLY_TEST_CHECK(is_almost_equal(mat3x3({2, 1, 3}, {5, 4, 6}, {8, 7, 9}), m2));

    mat3x3 m3 = mat3x3({0, 0, 5}, {0, -1, 0}, {-1, 0, 0}) * m;
    PLY_TEST_CHECK(
        is_almost_equal(mat3x3({-3, -2, 5}, {-6, -5, 20}, {-9, -8, 35}), m3));
}

} // namespace ply

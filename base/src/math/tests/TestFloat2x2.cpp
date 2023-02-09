/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-math.h>
#include <ply-test/TestSuite.h>

namespace ply {

#define PLY_TEST_CASE_PREFIX Float2x2_

namespace {
bool is_almost_equal(const mat2x2& m1, const mat2x2& m2) {
    return is_within(m1[0], m2[0], 1e-6f) && is_within(m1[1], m2[1], 1e-6f);
}
} // namespace

PLY_TEST_CASE("mat2x2 constructor") {
    mat2x2 m = {{1, 2}, {3, 4}};
    PLY_TEST_CHECK(m[0].x == 1 && m[0].y == 2);
    PLY_TEST_CHECK(m[1].x == 3 && m[1].y == 4);
}

PLY_TEST_CASE("mat2x2 element modification") {
    mat2x2 m = {{1, 2}, {3, 4}};

    m[0].x = 5;
    m[0].y = 6;
    m[1].x = 9;
    m[1].y = 10;

    PLY_TEST_CHECK(m[0].x == 5 && m[0].y == 6);
    PLY_TEST_CHECK(m[1].x == 9 && m[1].y == 10);
}

PLY_TEST_CASE("mat2x2 comparisons") {
    PLY_TEST_CHECK(mat2x2{{2, 3}, {4, 5}} == mat2x2{{2, 3}, {4, 5}});
    PLY_TEST_CHECK(mat2x2{{0, 0}, {0, 0}} == mat2x2{{0, -0.f}, {-0.f, 0}});
    PLY_TEST_CHECK(mat2x2{{5, 4}, {3, 2}} != mat2x2{{2, 3}, {4, 5}});

    PLY_TEST_CHECK(!(mat2x2{{2, 3}, {4, 5}} != mat2x2{{2, 3}, {4, 5}}));
    PLY_TEST_CHECK(!(mat2x2{{0, 0}, {0, 0}} != mat2x2{{0, -0.f}, {-0.f, 0}}));
    PLY_TEST_CHECK(!(mat2x2{{5, 4}, {3, 2}} == mat2x2{{2, 3}, {4, 5}}));
}

PLY_TEST_CASE("mat2x2 creation functions") {
    PLY_TEST_CHECK(mat2x2{{1, 0}, {0, 1}} == mat2x2::identity());
    PLY_TEST_CHECK(mat2x2{{2.f, 0}, {0, 2.f}} == mat2x2::make_scale(2.f));

    PLY_TEST_CHECK(
        is_almost_equal(mat2x2{{1.f, 0}, {0, 1.f}}, mat2x2::make_rotation(0)));
    PLY_TEST_CHECK(
        is_almost_equal(mat2x2{{-1.f, 0}, {0, -1.f}}, mat2x2::make_rotation(Pi)));
    PLY_TEST_CHECK(is_almost_equal(mat2x2{{0.f, 1.f}, {-1.f, 0.f}},
                                   mat2x2::make_rotation(Pi / 2)));

    PLY_TEST_CHECK(mat2x2::from_complex({1, 0}) == mat2x2::identity());
}

PLY_TEST_CASE("mat2x2 transposition") {
    PLY_TEST_CHECK(mat2x2{{1, 2}, {3, 4}} == mat2x2{{1, 3}, {2, 4}}.transposed());
    PLY_TEST_CHECK(mat2x2{{-4, -6}, {2, 4}} == mat2x2{{-4, 2}, {-6, 4}}.transposed());
}

PLY_TEST_CASE("mat2x2 transforms a vector") {
    vec2 v1 = {1, 2};
    PLY_TEST_CHECK(is_within(vec2{1, 2}, mat2x2({1, 0}, {0, 1}) * v1, 0));
    PLY_TEST_CHECK(is_within(vec2{2, 1}, mat2x2({0, 1}, {1, 0}) * v1, 0));
    PLY_TEST_CHECK(is_within(vec2{-1, -2}, mat2x2({-1, 0}, {0, -1}) * v1, 0));
}

PLY_TEST_CASE("mat2x2 matrix multiplication") {
    mat2x2 m = {{1, 2}, {3, 4}};

    mat2x2 m1 = mat2x2({1, 0}, {0, 1}) * m;
    PLY_TEST_CHECK(is_almost_equal(m, m1));

    mat2x2 m2 = mat2x2({0, 1}, {1, 0}) * m;
    PLY_TEST_CHECK(is_almost_equal(mat2x2{{2, 1}, {4, 3}}, m2));

    mat2x2 m3 = mat2x2({-1, 0}, {0, 1}) * m;
    PLY_TEST_CHECK(is_almost_equal(mat2x2{{-1, 2}, {-3, 4}}, m3));
}

} // namespace ply

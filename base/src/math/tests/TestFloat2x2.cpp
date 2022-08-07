/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-math/Base.h>
#include <ply-test/TestSuite.h>

namespace ply {

#define PLY_TEST_CASE_PREFIX Float2x2_

namespace {
bool isAlmostEqual(const Float2x2& m1, const Float2x2& m2) {
    return isNear(m1[0], m2[0], 1e-6f) && isNear(m1[1], m2[1], 1e-6f);
}
} // namespace

PLY_TEST_CASE("Float2x2 constructor") {
    Float2x2 m = {{1, 2}, {3, 4}};
    PLY_TEST_CHECK(m[0].x == 1 && m[0].y == 2);
    PLY_TEST_CHECK(m[1].x == 3 && m[1].y == 4);
}

PLY_TEST_CASE("Float2x2 element modification") {
    Float2x2 m = {{1, 2}, {3, 4}};

    m[0].x = 5;
    m[0].y = 6;
    m[1].x = 9;
    m[1].y = 10;

    PLY_TEST_CHECK(m[0].x == 5 && m[0].y == 6);
    PLY_TEST_CHECK(m[1].x == 9 && m[1].y == 10);
}

PLY_TEST_CASE("Float2x2 comparisons") {
    PLY_TEST_CHECK(Float2x2{{2, 3}, {4, 5}} == Float2x2{{2, 3}, {4, 5}});
    PLY_TEST_CHECK(Float2x2{{0, 0}, {0, 0}} == Float2x2{{0, -0.f}, {-0.f, 0}});
    PLY_TEST_CHECK(Float2x2{{5, 4}, {3, 2}} != Float2x2{{2, 3}, {4, 5}});

    PLY_TEST_CHECK(!(Float2x2{{2, 3}, {4, 5}} != Float2x2{{2, 3}, {4, 5}}));
    PLY_TEST_CHECK(!(Float2x2{{0, 0}, {0, 0}} != Float2x2{{0, -0.f}, {-0.f, 0}}));
    PLY_TEST_CHECK(!(Float2x2{{5, 4}, {3, 2}} == Float2x2{{2, 3}, {4, 5}}));
}

PLY_TEST_CASE("Float2x2 creation functions") {
    PLY_TEST_CHECK(Float2x2{{1, 0}, {0, 1}} == Float2x2::identity());
    PLY_TEST_CHECK(Float2x2{{2.f, 0}, {0, 2.f}} == Float2x2::makeScale(2.f));

    PLY_TEST_CHECK(isAlmostEqual(Float2x2{{1.f, 0}, {0, 1.f}}, Float2x2::makeRotation(0)));
    PLY_TEST_CHECK(isAlmostEqual(Float2x2{{-1.f, 0}, {0, -1.f}}, Float2x2::makeRotation(Pi)));
    PLY_TEST_CHECK(
        isAlmostEqual(Float2x2{{0.f, 1.f}, {-1.f, 0.f}}, Float2x2::makeRotation(Pi / 2)));

    PLY_TEST_CHECK(Float2x2::fromComplex({1, 0}) == Float2x2::identity());
}

PLY_TEST_CASE("Float2x2 transposition") {
    PLY_TEST_CHECK(Float2x2{{1, 2}, {3, 4}} == Float2x2{{1, 3}, {2, 4}}.transposed());
    PLY_TEST_CHECK(Float2x2{{-4, -6}, {2, 4}} == Float2x2{{-4, 2}, {-6, 4}}.transposed());
}

PLY_TEST_CASE("Float2x2 transforms a vector") {
    Float2 v1 = {1, 2};
    PLY_TEST_CHECK(isNear(Float2{1, 2}, Float2x2({1, 0}, {0, 1}) * v1, 0));
    PLY_TEST_CHECK(isNear(Float2{2, 1}, Float2x2({0, 1}, {1, 0}) * v1, 0));
    PLY_TEST_CHECK(isNear(Float2{-1, -2}, Float2x2({-1, 0}, {0, -1}) * v1, 0));
}

PLY_TEST_CASE("Float2x2 matrix multiplication") {
    Float2x2 m = {{1, 2}, {3, 4}};

    Float2x2 m1 = Float2x2({1, 0}, {0, 1}) * m;
    PLY_TEST_CHECK(isAlmostEqual(m, m1));

    Float2x2 m2 = Float2x2({0, 1}, {1, 0}) * m;
    PLY_TEST_CHECK(isAlmostEqual(Float2x2{{2, 1}, {4, 3}}, m2));

    Float2x2 m3 = Float2x2({-1, 0}, {0, 1}) * m;
    PLY_TEST_CHECK(isAlmostEqual(Float2x2{{-1, 2}, {-3, 4}}, m3));
}

} // namespace ply

/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-math/Base.h>
#include <ply-test/TestSuite.h>

namespace ply {

#define PLY_TEST_CASE_PREFIX Float3_

PLY_TEST_CASE("Float3 constructors") {
    Float3 v = {1};
    PLY_TEST_CHECK(v.x == 1 && v.y == 1 && v.z == 1);
    Float3 v2 = {1, 2, 5};
    PLY_TEST_CHECK(v2.x == 1 && v2.y == 2 && v2.z == 5);
    Float3 v3 = v2;
    PLY_TEST_CHECK(v3.x == 1 && v3.y == 2 && v3.z == 5);
}

PLY_TEST_CASE("Float3 conversions") {
    auto v = Float3{1, 2, 5}.to<IntVec3>();
    PLY_TEST_CHECK(v.x == 1 && v.y == 2 && v.z == 5);
    auto v2 = Float3{1, 2, 5}.to<Int3<s16>>();
    PLY_TEST_CHECK(v2.x == 1 && v2.y == 2 && v2.z == 5);
}

PLY_TEST_CASE("Float3 copy assignment") {
    Float3 v;
    v = Float3{1, 2, 5};
    PLY_TEST_CHECK(v.x == 1 && v.y == 2 && v.z == 5);
}

PLY_TEST_CASE("Float3 comparisons") {
    PLY_TEST_CHECK(Float3{1, 2, 5} == Float3{1, 2, 5});
    PLY_TEST_CHECK(Float3{0, 0, 0} == Float3{0, -0.f, 0});
    PLY_TEST_CHECK(Float3{1, 2, 5} != Float3{2, 1, 5});
    PLY_TEST_CHECK(!(Float3{1, 2, 5} != Float3{1, 2, 5}));
    PLY_TEST_CHECK(!(Float3{0, 0, 0} != Float3{0, -0.f, 0}));
    PLY_TEST_CHECK(!(Float3{1, 2, 5} == Float3{2, 1, 5}));
}

PLY_TEST_CASE("Float3 unary negation") {
    Float3 v = -Float3{1, 2, 5};
    PLY_TEST_CHECK(v.x == -1 && v.y == -2 && v.z == -5);
}

PLY_TEST_CASE("Float3 addition") {
    Float3 v = Float3{1, 2, 5} + Float3{3, 4, 0};
    PLY_TEST_CHECK(v.x == 4 && v.y == 6 && v.z == 5);
    Float3 v2 = Float3{1, 2, 5} + 3;
    PLY_TEST_CHECK(v2.x == 4 && v2.y == 5 && v2.z == 8);
    Float3 v3 = 3 + Float3{1, 2, 5};
    PLY_TEST_CHECK(v3.x == 4 && v3.y == 5 && v3.z == 8);
    v3 += Float3{1, 2, 5};
    PLY_TEST_CHECK(v3.x == 5 && v3.y == 7 && v3.z == 13);
    v3 += 1;
    PLY_TEST_CHECK(v3.x == 6 && v3.y == 8 && v3.z == 14);
}

PLY_TEST_CASE("Float3 subtraction") {
    Float3 v = Float3{3, 4, 5} - Float3{1, 2, 1};
    PLY_TEST_CHECK(v.x == 2 && v.y == 2 && v.z == 4);
    Float3 v2 = Float3{2, 3, 5} - 1;
    PLY_TEST_CHECK(v2.x == 1 && v2.y == 2 && v2.z == 4);
    Float3 v3 = 6 - Float3{1, 2, 1};
    PLY_TEST_CHECK(v3.x == 5 && v3.y == 4 && v3.z == 5);
    v3 -= Float3{1, 2, 0};
    PLY_TEST_CHECK(v3.x == 4 && v3.y == 2 && v3.z == 5);
    v3 -= 1;
    PLY_TEST_CHECK(v3.x == 3 && v3.y == 1 && v3.z == 4);
}

PLY_TEST_CASE("Float3 component-wise multiplication") {
    Float3 v = Float3{1, 2, 5} * Float3{3, 4, 2};
    PLY_TEST_CHECK(v.x == 3 && v.y == 8 && v.z == 10);
    Float3 v2 = Float3{1, 2, 5} * 3;
    PLY_TEST_CHECK(v2.x == 3 && v2.y == 6 && v2.z == 15);
    Float3 v3 = 3 * Float3{1, 2, 5};
    PLY_TEST_CHECK(v3.x == 3 && v3.y == 6 && v3.z == 15);
    v3 *= Float3{1, 2, 3};
    PLY_TEST_CHECK(v3.x == 3 && v3.y == 12 && v3.z == 45);
    v3 *= 2;
    PLY_TEST_CHECK(v3.x == 6 && v3.y == 24 && v3.z == 90);
}

PLY_TEST_CASE("Float3 component-wise division") {
    Float3 v = Float3{2, 6, 4} / Float3{2, 3, 1};
    PLY_TEST_CHECK(v.x == 1 && v.y == 2 && v.z == 4);
    Float3 v2 = Float3{4, 6, 2} / 2;
    PLY_TEST_CHECK(v2.x == 2 && v2.y == 3 && v2.z == 1);
    Float3 v3 = 8 / Float3{4, 2, 1};
    PLY_TEST_CHECK(v3.x == 2 && v3.y == 4 && v3.z == 8);
    v3 /= Float3{1, 2, 1};
    PLY_TEST_CHECK(v3.x == 2 && v3.y == 2 && v3.z == 8);
    v3 /= 2;
    PLY_TEST_CHECK(v3.x == 1 && v3.y == 1 && v3.z == 4);
}

PLY_TEST_CASE("Float3 lengths") {
    PLY_TEST_CHECK(Float3{3, 4, 2}.length2() == 29);
    PLY_TEST_CHECK((Float3{3, 4, 2}.length() - 5.385164f) < 1e-3f);
    PLY_TEST_CHECK(Float3{1, 0, 0}.isUnit());
    PLY_TEST_CHECK(!Float3{1, 1, 1}.isUnit());
    PLY_TEST_CHECK(Float3{999, 666, 333}.normalized().isUnit());
    PLY_TEST_CHECK(Float3{0}.safeNormalized().isUnit());
}

PLY_TEST_CASE("Float3 swizzles") {
    Float3 v = {4, 5, 6};
    PLY_TEST_CHECK(v.swizzle(2, 1) == Float2{6, 5});
    PLY_TEST_CHECK(v.swizzle(2, 0, 1) == Float3{6, 4, 5});
    PLY_TEST_CHECK(v.swizzle(1, 2, 1, 0) == Float4{5, 6, 5, 4});
}

PLY_TEST_CASE("Float3 dot product") {
    Float3 v1 = {2, 3, 1};
    Float3 v2 = {4, 5, 1};
    PLY_TEST_CHECK(dot(v1, v2) == 24);
    PLY_TEST_CHECK(v1.length2() == dot(v1, v1));
}

PLY_TEST_CASE("Float3 cross product") {
    PLY_TEST_CHECK(cross(Float3{1, 0, 0}, Float3{0, 1, 0}) == Float3{0, 0, 1});
    PLY_TEST_CHECK(cross(Float3{1, 0, 0}, Float3{0, 0, 1}) == Float3{0, -1, 0});
    PLY_TEST_CHECK(cross(Float3{1, 0, 0}, Float3{1, 0, 0}) == 0);
    PLY_TEST_CHECK(cross(Float3{2, 2, 1}, Float3{0, 4, 1}) == Float3{-2, -2, 8});
    PLY_TEST_CHECK(cross(Float3{2, 2, 2}, Float3{4, 0, 0}) == Float3{0, 8, -8});
}

PLY_TEST_CASE("Float3 clamp") {
    PLY_TEST_CHECK(clamp(Float3{2, 2, 2}, 0, 1) == Float3{1, 1, 1});
    PLY_TEST_CHECK(clamp(Float3{2, 0.5f, 0}, 0, 1) == Float3{1, 0.5f, 0});
    PLY_TEST_CHECK(clamp(Float3{-1, -1, -1}, 0, 1) == Float3{0, 0, 0});
    PLY_TEST_CHECK(clamp(Float3{3, 4, 5}, Float3{0, 1, 2}, Float3{1, 2, 3}) == Float3{1, 2, 3});
    PLY_TEST_CHECK(clamp(Float3{3, 1.5f, 1}, Float3{0, 1, 2}, Float3{1, 2, 3}) == Float3{1, 1.5f, 2});
    PLY_TEST_CHECK(clamp(Float3{-1, -1, -1}, Float3{0, 1, 2}, Float3{1, 2, 3}) == Float3{0, 1, 2});
}

PLY_TEST_CASE("Float3 abs") {
    PLY_TEST_CHECK(abs(Float3{1, 1, 1}) == Float3{1, 1, 1});
    PLY_TEST_CHECK(abs(Float3{-1, -1, -1}) == Float3{1, 1, 1});
    PLY_TEST_CHECK(abs(Float3{-2, 3, 0}) == Float3{2, 3, 0});
    PLY_TEST_CHECK(abs(Float3{0, 2, -3}) == Float3{0, 2, 3});
}

PLY_TEST_CASE("Float3 min") {
    PLY_TEST_CHECK(min(Float3{1, 0, 1}, Float3{0, 1, 0}) == Float3{0, 0, 0});
    PLY_TEST_CHECK(min(Float3{0, 1, 0}, Float3{1, 0, 1}) == Float3{0, 0, 0});
    PLY_TEST_CHECK(min(Float3{2, 2, 2}, Float3{3, 3, 3}) == Float3{2, 2, 2});
    PLY_TEST_CHECK(min(Float3{3, 3, 3}, Float3{2, 2, 2}) == Float3{2, 2, 2});
}

PLY_TEST_CASE("Float3 max") {
    PLY_TEST_CHECK(max(Float3{1, 0, 1}, Float3{0, 1, 0}) == Float3{1, 1, 1});
    PLY_TEST_CHECK(max(Float3{0, 1, 0}, Float3{1, 0, 1}) == Float3{1, 1, 1});
    PLY_TEST_CHECK(max(Float3{2, 2, 2}, Float3{3, 3, 3}) == Float3{3, 3, 3});
    PLY_TEST_CHECK(max(Float3{3, 3, 3}, Float3{2, 2, 2}) == Float3{3, 3, 3});
}

PLY_TEST_CASE("Float3 allLess") {
    PLY_TEST_CHECK(allLess(Float3{-1, -1, -1}, 0));
    PLY_TEST_CHECK(!allLess(Float3{1, -1, 1}, 0));
    PLY_TEST_CHECK(!allLess(Float3{-1, 1, -1}, 0));
    PLY_TEST_CHECK(!allLess(Float3{1, 1, 1}, 0));
    PLY_TEST_CHECK(!allLess(Float3{0, 0, 0}, 0));
    PLY_TEST_CHECK(allLess(Float3{0, 1, 3}, Float3{1, 2, 4}));
    PLY_TEST_CHECK(!allLess(Float3{2, 1, 0}, Float3{1, 2, 1}));
    PLY_TEST_CHECK(!allLess(Float3{0, 3, 3}, Float3{1, 4, 2}));
    PLY_TEST_CHECK(!allLess(Float3{3, 2, 3}, Float3{4, 1, 2}));
    PLY_TEST_CHECK(!allLess(Float3{3, 1, 2}, Float3{3, 1, 2}));
}

PLY_TEST_CASE("Float3 allLessOrEqual") {
    PLY_TEST_CHECK(allLessOrEqual(Float3{-1, -1, -1}, 0));
    PLY_TEST_CHECK(!allLessOrEqual(Float3{1, -1, 1}, 0));
    PLY_TEST_CHECK(!allLessOrEqual(Float3{-1, 1, -1}, 0));
    PLY_TEST_CHECK(!allLessOrEqual(Float3{1, 1, 1}, 0));
    PLY_TEST_CHECK(allLessOrEqual(Float3{0, 0, 0}, 0));
    PLY_TEST_CHECK(allLessOrEqual(Float3{0, 1, 3}, Float3{1, 2, 4}));
    PLY_TEST_CHECK(!allLessOrEqual(Float3{2, 1, 0}, Float3{1, 2, 1}));
    PLY_TEST_CHECK(!allLessOrEqual(Float3{0, 3, 3}, Float3{1, 4, 2}));
    PLY_TEST_CHECK(!allLessOrEqual(Float3{3, 2, 3}, Float3{4, 1, 2}));
    PLY_TEST_CHECK(allLessOrEqual(Float3{3, 1, 2}, Float3{3, 1, 2}));
}

PLY_TEST_CASE("Float3 quantizeNearest") {
    PLY_TEST_CHECK(quantizeNearest(Float3{0.3f, 1.4f, 0.8f}, 2) == Float3{0, 2, 0});
    PLY_TEST_CHECK(quantizeNearest(Float3{-0.3f, 1.4f, 0.8f}, 1) == Float3{0, 1, 1});
    PLY_TEST_CHECK(quantizeNearest(Float3{0.3f, -1.4f, -0.8f}, 1) == Float3{0, -1, -1});
    PLY_TEST_CHECK(quantizeNearest(Float3{-0.3f, 1.4f, 0.8f}, 0.5f) == Float3{-0.5f, 1.5f, 1});
}

PLY_TEST_CASE("Float3 quantizeUp") {
    PLY_TEST_CHECK(quantizeUp(Float3{0.3f, 1.4f, 0.8f}, 2) == Float3{2, 2, 2});
    PLY_TEST_CHECK(quantizeUp(Float3{-0.3f, 1.4f, 0.8f}, 1) == Float3{0, 2, 1});
    PLY_TEST_CHECK(quantizeUp(Float3{0.3f, -1.4f, -0.8f}, 1) == Float3{1, -1, 0});
    PLY_TEST_CHECK(quantizeUp(Float3{-0.3f, 1.4f, 0.8f}, 0.5f) == Float3{0, 1.5f, 1});
}

PLY_TEST_CASE("Float3 quantizeDown") {
    PLY_TEST_CHECK(quantizeDown(Float3{0.3f, 1.4f, 0.8f}, 2) == Float3{0, 0, 0});
    PLY_TEST_CHECK(quantizeDown(Float3{-0.3f, 1.4f, 0.8f}, 1) == Float3{-1, 1, 0});
    PLY_TEST_CHECK(quantizeDown(Float3{0.3f, -1.4f, -0.8f}, 1) == Float3{0, -2, -1});
    PLY_TEST_CHECK(quantizeDown(Float3{-0.3f, 1.4f, 0.8f}, 0.5f) == Float3{-0.5f, 1, 0.5f});
}

PLY_TEST_CASE("Float3 isQuantized") {
    PLY_TEST_CHECK(isQuantized(Float3{0, 1, 2}, 1));
    PLY_TEST_CHECK(!isQuantized(Float3{0.5f, 0, 2}, 1));
    PLY_TEST_CHECK(!isQuantized(Float3{1, -0.5f, 0}, 1));
    PLY_TEST_CHECK(!isQuantized(Float3{0, 1, 0.5f}, 1));
    PLY_TEST_CHECK(isQuantized(Float3{0, 1, 2}, 0.5f));
    PLY_TEST_CHECK(isQuantized(Float3{0.5f, -0.f, -1.5f}, 0.5f));
    PLY_TEST_CHECK(isQuantized(Float3{1, -0.5f, 0.f}, 0.5f));
    PLY_TEST_CHECK(!isQuantized(Float3{-0.5f, 0.3f, 0.f}, 0.5f));
    PLY_TEST_CHECK(!isQuantized(Float3{0, 1, 2}, 2));
    PLY_TEST_CHECK(isQuantized(Float3{4, 8, -2}, 2));
}

} // namespace ply

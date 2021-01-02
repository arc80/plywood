/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-math/Base.h>
#include <ply-test/TestSuite.h>

namespace ply {

#define PLY_TEST_CASE_PREFIX Float4_

PLY_TEST_CASE("Float4 constructors") {
    Float4 v = {1};
    PLY_TEST_CHECK(v.x == 1 && v.y == 1 && v.z == 1 && v.w == 1);
    Float4 v2 = {1, 2, 5, 0};
    PLY_TEST_CHECK(v2.x == 1 && v2.y == 2 && v2.z == 5 && v2.w == 0);
    Float4 v3 = v2;
    PLY_TEST_CHECK(v3.x == 1 && v3.y == 2 && v3.z == 5 && v3.w == 0);
}

PLY_TEST_CASE("Float4 conversions") {
    auto v = Float4{1, 2, 5, 0}.to<IntVec4>();
    PLY_TEST_CHECK(v.x == 1 && v.y == 2 && v.z == 5 && v.w == 0);
    auto v2 = Float4{1, 2, 5, 0}.to<Int4<s16>>();
    PLY_TEST_CHECK(v2.x == 1 && v2.y == 2 && v2.z == 5 && v2.w == 0);
}

PLY_TEST_CASE("Float4 copy assignment") {
    Float4 v;
    v = Float4{1, 2, 5, 0};
    PLY_TEST_CHECK(v.x == 1 && v.y == 2 && v.z == 5 && v.w == 0);
}

PLY_TEST_CASE("Float4 comparisons") {
    PLY_TEST_CHECK(Float4{1, 2, 5, 0} == Float4{1, 2, 5, 0});
    PLY_TEST_CHECK(Float4{0, 0, 0, -0.f} == Float4{0, -0.f, 0, 0});
    PLY_TEST_CHECK(Float4{1, 2, 5, 0} != Float4{2, 1, 5, 0});
    PLY_TEST_CHECK(!(Float4{1, 2, 5, 0} != Float4{1, 2, 5, 0}));
    PLY_TEST_CHECK(!(Float4{0, 0, 0, -0.f} != Float4{0, -0.f, 0, 0}));
    PLY_TEST_CHECK(!(Float4{1, 2, 5, 0} == Float4{2, 1, 5, 0}));
}

PLY_TEST_CASE("Float4 isNear") {
    PLY_TEST_CHECK(isNear(Float4{4}, Float4{4}, 1e-6f));
    PLY_TEST_CHECK(isNear(Float4{5}, Float4{5}, 0));
    PLY_TEST_CHECK(!isNear(Float4{5}, Float4{6}, 0));
    PLY_TEST_CHECK(isNear(Float4{1, 0, 2, 3}, Float4{0.9999f, 0.0001f, 1.9999f, 3.0001f}, 0.001f));
    PLY_TEST_CHECK(!isNear(Float4{1, 0, 2, 3}, Float4{0.999f, 0.001f, 1.9999f, 3.0001f}, 0.001f));
}

PLY_TEST_CASE("Float4 unary negation") {
    PLY_TEST_CHECK(-Float4{1, 2, 5, 0} == Float4{-1, -2, -5, 0});
}

PLY_TEST_CASE("Float4 addition") {
    PLY_TEST_CHECK(Float4{1, 2, 5, 2} + Float4{3, 4, 0, 1} == Float4{4, 6, 5, 3});
    PLY_TEST_CHECK(Float4{1, 2, 5, 2} + 3 == Float4{4, 5, 8, 5});
    PLY_TEST_CHECK(3 + Float4{1, 2, 5, 2} == Float4{4, 5, 8, 5});
    Float4 v = {4, 5, 8, 5};
    v += Float4{1, 2, 5, 1};
    PLY_TEST_CHECK(v == Float4{5, 7, 13, 6});
    v += 1;
    PLY_TEST_CHECK(v == Float4{6, 8, 14, 7});
}

PLY_TEST_CASE("Float4 subtraction") {
    PLY_TEST_CHECK(Float4{3, 4, 5, 7} - Float4{1, 2, 1, 2} == Float4{2, 2, 4, 5});
    PLY_TEST_CHECK(Float4{2, 3, 5, 7} - 1 == Float4{1, 2, 4, 6});
    PLY_TEST_CHECK(6 - Float4{1, 2, 1, 0} == Float4{5, 4, 5, 6});
    Float4 v = {5, 4, 5, 6};
    v -= Float4{1, 2, 0, 2};
    PLY_TEST_CHECK(v == Float4{4, 2, 5, 4});
    v -= 1;
    PLY_TEST_CHECK(v == Float4{3, 1, 4, 3});
}

PLY_TEST_CASE("Float4 component-wise multiplication") {
    PLY_TEST_CHECK(Float4{1, 2, 5, 0} * Float4{3, 4, 2, 5} == Float4{3, 8, 10, 0});
    PLY_TEST_CHECK(Float4{1, 2, 5, 0} * 3 == Float4{3, 6, 15, 0});
    PLY_TEST_CHECK(3 * Float4{1, 2, 5, 0} == Float4{3, 6, 15, 0});
    Float4 v = {3, 6, 15, 0};
    v *= Float4{1, 2, 3, 7};
    PLY_TEST_CHECK(v == Float4{3, 12, 45, 0});
    v *= 2;
    PLY_TEST_CHECK(v == Float4{6, 24, 90, 0});
}

PLY_TEST_CASE("Float4 component-wise division") {
    PLY_TEST_CHECK(Float4{2, 6, 4, 9} / Float4{2, 3, 1, 3} == Float4{1, 2, 4, 3});
    PLY_TEST_CHECK(Float4{4, 6, 2, 0} / 2 == Float4{2, 3, 1, 0});
    PLY_TEST_CHECK(8 / Float4{4, 2, 1, 8} == Float4{2, 4, 8, 1});
    Float4 v = {2, 4, 8, 1};
    v /= Float4{1, 2, 1, 1};
    PLY_TEST_CHECK(v == Float4{2, 2, 8, 1});
    v /= 2;
    PLY_TEST_CHECK(v == Float4{1, 1, 4, 0.5f});
}

PLY_TEST_CASE("Float4 lengths") {
    PLY_TEST_CHECK(Float4{3, 4, 2, -1}.length2() == 30);
    PLY_TEST_CHECK((Float4{3, 4, 2, -1}.length() - 5.477225f) < 1e-3f);
    PLY_TEST_CHECK(Float4{1, 0, 0, 0}.isUnit());
    PLY_TEST_CHECK(!Float4{1, 1, 1, 1}.isUnit());
    PLY_TEST_CHECK(Float4{999, 666, 333, -444}.normalized().isUnit());
    PLY_TEST_CHECK(Float4{0}.safeNormalized().isUnit());
}

PLY_TEST_CASE("Float4 swizzles") {
    Float4 v = {4, 5, 6, 7};
    PLY_TEST_CHECK(v.swizzle(3, 1) == Float2{7, 5});
    PLY_TEST_CHECK(v.swizzle(2, 0, 3) == Float3{6, 4, 7});
    PLY_TEST_CHECK(v.swizzle(1, 2, 3, 0) == Float4{5, 6, 7, 4});
}

PLY_TEST_CASE("Float4 dot product") {
    Float4 v1 = {2, 3, 1, 3};
    Float4 v2 = {4, 5, 1, 0};
    PLY_TEST_CHECK(dot(v1, v2) == 24);
    PLY_TEST_CHECK(v1.length2() == dot(v1, v1));
}

PLY_TEST_CASE("Float4 clamp") {
    PLY_TEST_CHECK(clamp(Float4{2, 2, 2, 2}, 0, 1) == Float4{1, 1, 1, 1});
    PLY_TEST_CHECK(clamp(Float4{2, 0.5f, 0, 1.5f}, 0, 1) == Float4{1, 0.5f, 0, 1});
    PLY_TEST_CHECK(clamp(Float4{-1, -1, -1, -1}, 0, 1) == Float4{0, 0, 0, 0});
    PLY_TEST_CHECK(clamp(Float4{3, 4, 5, 6}, Float4{0, 1, 2, 3}, Float4{1, 2, 3, 4}) ==
                   Float4{1, 2, 3, 4});
    PLY_TEST_CHECK(clamp(Float4{3, 1.5f, 1, 3.5f}, Float4{0, 1, 2, 3}, Float4{1, 2, 3, 4}) ==
                   Float4{1, 1.5f, 2, 3.5f});
    PLY_TEST_CHECK(clamp(Float4{-1, -1, -1, -1}, Float4{0, 1, 2, 3}, Float4{1, 2, 3, 4}) ==
                   Float4{0, 1, 2, 3});
}

PLY_TEST_CASE("Float4 abs") {
    PLY_TEST_CHECK(abs(Float4{1, 1, 1, 1}) == Float4{1, 1, 1, 1});
    PLY_TEST_CHECK(abs(Float4{-1, -1, -1, -1}) == Float4{1, 1, 1, 1});
    PLY_TEST_CHECK(abs(Float4{-2, 3, 0, -1}) == Float4{2, 3, 0, 1});
    PLY_TEST_CHECK(abs(Float4{0, 2, -3, 1}) == Float4{0, 2, 3, 1});
}

PLY_TEST_CASE("Float4 min") {
    PLY_TEST_CHECK(min(Float4{1, 0, 1, 0}, Float4{0, 1, 0, 1}) == Float4{0, 0, 0, 0});
    PLY_TEST_CHECK(min(Float4{0, 1, 0, 1}, Float4{1, 0, 1, 0}) == Float4{0, 0, 0, 0});
    PLY_TEST_CHECK(min(Float4{2, 2, 2, 2}, Float4{3, 3, 3, 3}) == Float4{2, 2, 2, 2});
    PLY_TEST_CHECK(min(Float4{3, 3, 3, 3}, Float4{2, 2, 2, 2}) == Float4{2, 2, 2, 2});
}

PLY_TEST_CASE("Float4 max") {
    PLY_TEST_CHECK(max(Float4{1, 0, 1, 0}, Float4{0, 1, 0, 1}) == Float4{1, 1, 1, 1});
    PLY_TEST_CHECK(max(Float4{0, 1, 0, 1}, Float4{1, 0, 1, 0}) == Float4{1, 1, 1, 1});
    PLY_TEST_CHECK(max(Float4{2, 2, 2, 2}, Float4{3, 3, 3, 3}) == Float4{3, 3, 3, 3});
    PLY_TEST_CHECK(max(Float4{3, 3, 3, 3}, Float4{2, 2, 2, 2}) == Float4{3, 3, 3, 3});
}

PLY_TEST_CASE("Float4 comparisons (all)") {
    PLY_TEST_CHECK(all(Float4{-1, -1, -1, -1} < 0));
    PLY_TEST_CHECK(!all(Float4{1, -1, 1, -1} <= 0));
    PLY_TEST_CHECK(!all(0 > Float4{-1, 1, -1, 1}));
    PLY_TEST_CHECK(!all(0 >= Float4{1, 1, 1, 1}));
    PLY_TEST_CHECK(!all(Float4{0, 0, 0, 0} < 0));
    PLY_TEST_CHECK(all(Float4{0, 0, 0, 0} <= 0));
    PLY_TEST_CHECK(all(Float4{1, 2, 4, 3} > Float4{0, 1, 3, 2}));
    PLY_TEST_CHECK(!all(Float4{1, 2, 1, 3} >= Float4{2, 1, 0, 2}));
    PLY_TEST_CHECK(!all(Float4{0, 3, 3, 2} < Float4{1, 4, 2, 3}));
    PLY_TEST_CHECK(!all(Float4{3, 2, 3, 2} <= Float4{4, 1, 2, 3}));
    PLY_TEST_CHECK(!all(Float4{3, 1, 2, 0} > Float4{3, 1, 2, 0}));
    PLY_TEST_CHECK(all(Float4{3, 1, 2, 0} >= Float4{3, 1, 2, 0}));
}

PLY_TEST_CASE("Float4 comparisons (all)") {
    PLY_TEST_CHECK(!any(Float4{-1, -1, -1, -1} >= 0));
    PLY_TEST_CHECK(any(Float4{1, -1, 1, -1} > 0));
    PLY_TEST_CHECK(any(0 <= Float4{-1, 1, -1, 1}));
    PLY_TEST_CHECK(any(0 < Float4{1, 1, 1, 1}));
    PLY_TEST_CHECK(any(Float4{0, 0, 0, 0} >= 0));
    PLY_TEST_CHECK(!any(Float4{0, 0, 0, 0} > 0));
    PLY_TEST_CHECK(!any(Float4{1, 2, 4, 3} <= Float4{0, 1, 3, 2}));
    PLY_TEST_CHECK(any(Float4{1, 2, 1, 3} < Float4{2, 1, 0, 2}));
    PLY_TEST_CHECK(any(Float4{0, 3, 3, 2} >= Float4{1, 4, 2, 3}));
    PLY_TEST_CHECK(any(Float4{3, 2, 3, 2} > Float4{4, 1, 2, 3}));
    PLY_TEST_CHECK(any(Float4{3, 1, 2, 0} <= Float4{3, 1, 2, 0}));
    PLY_TEST_CHECK(!any(Float4{3, 1, 2, 0} < Float4{3, 1, 2, 0}));
}

PLY_TEST_CASE("Float4 quantizeNearest") {
    PLY_TEST_CHECK(quantizeNearest(Float4{0.3f, 1.4f, 0.8f, 1.2f}, 2) == Float4{0, 2, 0, 2});
    PLY_TEST_CHECK(quantizeNearest(Float4{-0.3f, 1.4f, 0.8f, -1.2f}, 1) == Float4{0, 1, 1, -1});
    PLY_TEST_CHECK(quantizeNearest(Float4{0.3f, -1.4f, -0.8f, 1.2f}, 1) == Float4{0, -1, -1, 1});
    PLY_TEST_CHECK(quantizeNearest(Float4{-0.3f, 1.4f, 0.8f, -1.2f}, 0.5f) ==
                   Float4{-0.5f, 1.5f, 1, -1});
}

PLY_TEST_CASE("Float4 quantizeUp") {
    PLY_TEST_CHECK(quantizeUp(Float4{0.3f, 1.4f, 0.8f, 1.2f}, 2) == Float4{2, 2, 2, 2});
    PLY_TEST_CHECK(quantizeUp(Float4{-0.3f, 1.4f, 0.8f, -1.2f}, 1) == Float4{0, 2, 1, -1});
    PLY_TEST_CHECK(quantizeUp(Float4{0.3f, -1.4f, -0.8f, 1.2f}, 1) == Float4{1, -1, 0, 2});
    PLY_TEST_CHECK(quantizeUp(Float4{-0.3f, 1.4f, 0.8f, -1.2f}, 0.5f) == Float4{0, 1.5f, 1, -1});
}

PLY_TEST_CASE("Float4 quantizeDown") {
    PLY_TEST_CHECK(quantizeDown(Float4{0.3f, 1.4f, 0.8f, 1.2f}, 2) == Float4{0, 0, 0, 0});
    PLY_TEST_CHECK(quantizeDown(Float4{-0.3f, 1.4f, 0.8f, -1.2f}, 1) == Float4{-1, 1, 0, -2});
    PLY_TEST_CHECK(quantizeDown(Float4{0.3f, -1.4f, -0.8f, 1.2f}, 1) == Float4{0, -2, -1, 1});
    PLY_TEST_CHECK(quantizeDown(Float4{-0.3f, 1.4f, 0.8f, -1.2f}, 0.5f) ==
                   Float4{-0.5f, 1, 0.5f, -1.5f});
}

PLY_TEST_CASE("Float4 isQuantized") {
    PLY_TEST_CHECK(isQuantized(Float4{0, 1, 2}, 1));
    PLY_TEST_CHECK(!isQuantized(Float4{0.5f, 0, 2}, 1));
    PLY_TEST_CHECK(!isQuantized(Float4{1, -0.5f, 0}, 1));
    PLY_TEST_CHECK(!isQuantized(Float4{0, 1, 0.5f}, 1));
    PLY_TEST_CHECK(isQuantized(Float4{0, 1, 2}, 0.5f));
    PLY_TEST_CHECK(isQuantized(Float4{0.5f, -0.f, -1.5f}, 0.5f));
    PLY_TEST_CHECK(isQuantized(Float4{1, -0.5f, 0.f}, 0.5f));
    PLY_TEST_CHECK(!isQuantized(Float4{-0.5f, 0.3f, 0.f}, 0.5f));
    PLY_TEST_CHECK(!isQuantized(Float4{0, 1, 2}, 2));
    PLY_TEST_CHECK(isQuantized(Float4{4, 8, -2}, 2));
}

} // namespace ply

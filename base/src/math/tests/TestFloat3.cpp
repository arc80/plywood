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

PLY_TEST_CASE("Float3 isNear") {
    PLY_TEST_CHECK(isNear(Float3{4}, Float3{4}, 1e-6f));
    PLY_TEST_CHECK(isNear(Float3{5}, Float3{5}, 0));
    PLY_TEST_CHECK(!isNear(Float3{5}, Float3{6}, 0));
    PLY_TEST_CHECK(isNear(Float3{1, 0, 2}, Float3{0.9999f, 0.0001f, 1.9999f}, 1e-3f));
    PLY_TEST_CHECK(!isNear(Float3{1, 0, 2}, Float3{0.999f, 0.001f, 1.9999f}, 1e-3f));
}

PLY_TEST_CASE("Float3 unary negation") {
    PLY_TEST_CHECK(-Float3{1, 2, 5} == Float3{-1, -2, -5});
}

PLY_TEST_CASE("Float3 addition") {
    PLY_TEST_CHECK(Float3{1, 2, 5} + Float3{3, 4, 0} == Float3{4, 6, 5});
    PLY_TEST_CHECK(Float3{1, 2, 5} + 3 == Float3{4, 5, 8});
    PLY_TEST_CHECK(3 + Float3{1, 2, 5} == Float3{4, 5, 8});
    Float3 v = {4, 5, 8};
    v += Float3{1, 2, 5};
    PLY_TEST_CHECK(v == Float3{5, 7, 13});
    v += 1;
    PLY_TEST_CHECK(v == Float3{6, 8, 14});
}

PLY_TEST_CASE("Float3 subtraction") {
    PLY_TEST_CHECK(Float3{3, 4, 5} - Float3{1, 2, 1} == Float3{2, 2, 4});
    PLY_TEST_CHECK(Float3{2, 3, 5} - 1 == Float3{1, 2, 4});
    PLY_TEST_CHECK(6 - Float3{1, 2, 1} == Float3{5, 4, 5});
    Float3 v = {5, 4, 5};
    v -= Float3{1, 2, 0};
    PLY_TEST_CHECK(v == Float3{4, 2, 5});
    v -= 1;
    PLY_TEST_CHECK(v == Float3{3, 1, 4});
}

PLY_TEST_CASE("Float3 component-wise multiplication") {
    PLY_TEST_CHECK(Float3{1, 2, 5} * Float3{3, 4, 2} == Float3{3, 8, 10});
    PLY_TEST_CHECK(Float3{1, 2, 5} * 3 == Float3{3, 6, 15});
    PLY_TEST_CHECK(3 * Float3{1, 2, 5} == Float3{3, 6, 15});
    Float3 v = {3, 6, 15};
    v *= Float3{1, 2, 3};
    PLY_TEST_CHECK(v == Float3{3, 12, 45});
    v *= 2;
    PLY_TEST_CHECK(v == Float3{6, 24, 90});
}

PLY_TEST_CASE("Float3 component-wise division") {
    PLY_TEST_CHECK(Float3{2, 6, 4} / Float3{2, 3, 1} == Float3{1, 2, 4});
    PLY_TEST_CHECK(Float3{4, 6, 2} / 2 == Float3{2, 3, 1});
    PLY_TEST_CHECK(8 / Float3{4, 2, 1} == Float3{2, 4, 8});
    Float3 v = {2, 4, 8};
    v /= Float3{1, 2, 1};
    PLY_TEST_CHECK(v == Float3{2, 2, 8});
    v /= 2;
    PLY_TEST_CHECK(v == Float3{1, 1, 4});
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

PLY_TEST_CASE("Float3 pow") {
    PLY_TEST_CHECK(pow(Float3{1, 2, 1}, 2) == Float3{1, 4, 1});
    PLY_TEST_CHECK(pow(2, Float3{1, 2, 0}) == Float3{2, 4, 1});
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

PLY_TEST_CASE("Float3 comparisons (all)") {
    PLY_TEST_CHECK(all(Float3{-1, -1, -1} < 0));
    PLY_TEST_CHECK(!all(Float3{1, -1, 1} <= 0));
    PLY_TEST_CHECK(!all(0 > Float3{-1, 1, -1}));
    PLY_TEST_CHECK(!all(0 >= Float3{1, 1, 1}));
    PLY_TEST_CHECK(!all(Float3{0, 0, 0} < 0));
    PLY_TEST_CHECK(all(Float3{0, 0, 0} <= 0));
    PLY_TEST_CHECK(all(Float3{1, 2, 4} > Float3{0, 1, 3}));
    PLY_TEST_CHECK(!all(Float3{1, 2, 1} >= Float3{2, 1, 0}));
    PLY_TEST_CHECK(!all(Float3{0, 3, 3} < Float3{1, 4, 2}));
    PLY_TEST_CHECK(!all(Float3{3, 2, 3} <= Float3{4, 1, 2}));
    PLY_TEST_CHECK(!all(Float3{3, 1, 2} > Float3{3, 1, 2}));
    PLY_TEST_CHECK(all(Float3{3, 1, 2} >= Float3{3, 1, 2}));
}

PLY_TEST_CASE("Float3 comparisons (all)") {
    PLY_TEST_CHECK(!any(Float3{-1, -1, -1} >= 0));
    PLY_TEST_CHECK(any(Float3{1, -1, 1} > 0));
    PLY_TEST_CHECK(any(0 <= Float3{-1, 1, -1}));
    PLY_TEST_CHECK(any(0 < Float3{1, 1, 1}));
    PLY_TEST_CHECK(any(Float3{0, 0, 0} >= 0));
    PLY_TEST_CHECK(!any(Float3{0, 0, 0} > 0));
    PLY_TEST_CHECK(!any(Float3{1, 2, 4} <= Float3{0, 1, 3}));
    PLY_TEST_CHECK(any(Float3{1, 2, 1} < Float3{2, 1, 0}));
    PLY_TEST_CHECK(any(Float3{0, 3, 3} >= Float3{1, 4, 2}));
    PLY_TEST_CHECK(any(Float3{3, 2, 3} > Float3{4, 1, 2}));
    PLY_TEST_CHECK(any(Float3{3, 1, 2} <= Float3{3, 1, 2}));
    PLY_TEST_CHECK(!any(Float3{3, 1, 2} < Float3{3, 1, 2}));
}

PLY_TEST_CASE("Float3 roundNearest") {
    PLY_TEST_CHECK(roundNearest(Float3{0.3f, 1.4f, 0.8f}, 2) == Float3{0, 2, 0});
    PLY_TEST_CHECK(roundNearest(Float3{-0.3f, 1.4f, 0.8f}, 1) == Float3{0, 1, 1});
    PLY_TEST_CHECK(roundNearest(Float3{0.3f, -1.4f, -0.8f}, 1) == Float3{0, -1, -1});
    PLY_TEST_CHECK(roundNearest(Float3{-0.3f, 1.4f, 0.8f}, 0.5f) == Float3{-0.5f, 1.5f, 1});
}

PLY_TEST_CASE("Float3 roundUp") {
    PLY_TEST_CHECK(roundUp(Float3{0.3f, 1.4f, 0.8f}, 2) == Float3{2, 2, 2});
    PLY_TEST_CHECK(roundUp(Float3{-0.3f, 1.4f, 0.8f}, 1) == Float3{0, 2, 1});
    PLY_TEST_CHECK(roundUp(Float3{0.3f, -1.4f, -0.8f}, 1) == Float3{1, -1, 0});
    PLY_TEST_CHECK(roundUp(Float3{-0.3f, 1.4f, 0.8f}, 0.5f) == Float3{0, 1.5f, 1});
}

PLY_TEST_CASE("Float3 roundDown") {
    PLY_TEST_CHECK(roundDown(Float3{0.3f, 1.4f, 0.8f}, 2) == Float3{0, 0, 0});
    PLY_TEST_CHECK(roundDown(Float3{-0.3f, 1.4f, 0.8f}, 1) == Float3{-1, 1, 0});
    PLY_TEST_CHECK(roundDown(Float3{0.3f, -1.4f, -0.8f}, 1) == Float3{0, -2, -1});
    PLY_TEST_CHECK(roundDown(Float3{-0.3f, 1.4f, 0.8f}, 0.5f) == Float3{-0.5f, 1, 0.5f});
}

PLY_TEST_CASE("Float3 isRounded") {
    PLY_TEST_CHECK(isRounded(Float3{0, 1, 2}, 1));
    PLY_TEST_CHECK(!isRounded(Float3{0.5f, 0, 2}, 1));
    PLY_TEST_CHECK(!isRounded(Float3{1, -0.5f, 0}, 1));
    PLY_TEST_CHECK(!isRounded(Float3{0, 1, 0.5f}, 1));
    PLY_TEST_CHECK(isRounded(Float3{0, 1, 2}, 0.5f));
    PLY_TEST_CHECK(isRounded(Float3{0.5f, -0.f, -1.5f}, 0.5f));
    PLY_TEST_CHECK(isRounded(Float3{1, -0.5f, 0.f}, 0.5f));
    PLY_TEST_CHECK(!isRounded(Float3{-0.5f, 0.3f, 0.f}, 0.5f));
    PLY_TEST_CHECK(!isRounded(Float3{0, 1, 2}, 2));
    PLY_TEST_CHECK(isRounded(Float3{4, 8, -2}, 2));
}

} // namespace ply

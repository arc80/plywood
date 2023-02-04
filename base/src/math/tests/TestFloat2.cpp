/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-math/Base.h>
#include <ply-test/TestSuite.h>

namespace ply {

#define PLY_TEST_CASE_PREFIX Float2_

PLY_TEST_CASE("Float2 constructors") {
    Float2 v = {1};
    PLY_TEST_CHECK(v.x == 1 && v.y == 1);
    Float2 v2 = {1, 2};
    PLY_TEST_CHECK(v2.x == 1 && v2.y == 2);
    Float2 v3 = v2;
    PLY_TEST_CHECK(v3.x == 1 && v3.y == 2);
}

PLY_TEST_CASE("Float2 conversions") {
    auto v = Float2{1, 2}.to<IntVec2>();
    PLY_TEST_CHECK(v.x == 1 && v.y == 2);
    auto v2 = Float2{1, 2}.to<Int2<s16>>();
    PLY_TEST_CHECK(v2.x == 1 && v2.y == 2);
}

PLY_TEST_CASE("Float2 copy assignment") {
    Float2 v;
    v = Float2{1, 2};
    PLY_TEST_CHECK(v.x == 1 && v.y == 2);
}

PLY_TEST_CASE("Float2 comparisons") {
    PLY_TEST_CHECK(Float2{1, 2} == Float2{1, 2});
    PLY_TEST_CHECK(Float2{0, 0} == Float2{0, -0.f});
    PLY_TEST_CHECK(Float2{1, 2} != Float2{2, 1});
    PLY_TEST_CHECK(!(Float2{1, 2} != Float2{1, 2}));
    PLY_TEST_CHECK(!(Float2{0, 0} != Float2{0, -0.f}));
    PLY_TEST_CHECK(!(Float2{1, 2} == Float2{2, 1}));
}

PLY_TEST_CASE("Float2 isNear") {
    PLY_TEST_CHECK(isNear(Float2{4}, Float2{4}, 1e-6f));
    PLY_TEST_CHECK(isNear(Float2{5}, Float2{5}, 0));
    PLY_TEST_CHECK(!isNear(Float2{5}, Float2{6}, 0));
    PLY_TEST_CHECK(isNear(Float2{1, 0}, Float2{0.9999f, 0.0001f}, 1e-3f));
    PLY_TEST_CHECK(!isNear(Float2{1, 0}, Float2{0.999f, 0.001f}, 1e-3f));
}

PLY_TEST_CASE("Float2 unary negation") {
    PLY_TEST_CHECK(-Float2{1, 2} == Float2{-1, -2});
}

PLY_TEST_CASE("Float2 addition") {
    PLY_TEST_CHECK(Float2{1, 2} + Float2{3, 4} == Float2{4, 6});
    PLY_TEST_CHECK(Float2{1, 2} + 3 == Float2{4, 5});
    PLY_TEST_CHECK(3 + Float2{1, 2} == Float2{4, 5});
    Float2 v = {4, 5};
    v += Float2{1, 2};
    PLY_TEST_CHECK(v == Float2{5, 7});
    v += 1;
    PLY_TEST_CHECK(v == Float2{6, 8});
}

PLY_TEST_CASE("Float2 subtraction") {
    PLY_TEST_CHECK(Float2{3, 4} - Float2{1, 2} == Float2{2, 2});
    PLY_TEST_CHECK(Float2{2, 3} - 1 == Float2{1, 2});
    PLY_TEST_CHECK(6 - Float2{1, 2} == Float2{5, 4});
    Float2 v = {5, 4};
    v -= Float2{1, 2};
    PLY_TEST_CHECK(v.x == 4 && v.y == 2);
    v -= 1;
    PLY_TEST_CHECK(v.x == 3 && v.y == 1);
}

PLY_TEST_CASE("Float2 component-wise multiplication") {
    PLY_TEST_CHECK(Float2{1, 2} * Float2{3, 4} == Float2{3, 8});
    PLY_TEST_CHECK(Float2{1, 2} * 3 == Float2{3, 6});
    PLY_TEST_CHECK(3 * Float2{1, 2} == Float2{3, 6});
    Float2 v = {3, 6};
    v *= Float2{1, 2};
    PLY_TEST_CHECK(v.x == 3 && v.y == 12);
    v *= 2;
    PLY_TEST_CHECK(v.x == 6 && v.y == 24);
}

PLY_TEST_CASE("Float2 component-wise division") {
    PLY_TEST_CHECK(Float2{2, 6} / Float2{2, 3} == Float2{1, 2});
    PLY_TEST_CHECK(Float2{4, 6} / 2 == Float2{2, 3});
    PLY_TEST_CHECK(8 / Float2{4, 2} == Float2{2, 4});
    Float2 v = {2, 4};
    v /= Float2{1, 2};
    PLY_TEST_CHECK(v.x == 2 && v.y == 2);
    v /= 2;
    PLY_TEST_CHECK(v.x == 1 && v.y == 1);
}

PLY_TEST_CASE("Float2 lengths") {
    PLY_TEST_CHECK(Float2{3, 4}.length2() == 25);
    PLY_TEST_CHECK(Float2{3, 4}.length() == 5);
    PLY_TEST_CHECK(Float2{1, 0}.isUnit());
    PLY_TEST_CHECK(!Float2{1, 1}.isUnit());
    PLY_TEST_CHECK(Float2{999, 666}.normalized().isUnit());
    PLY_TEST_CHECK(Float2{0}.safeNormalized().isUnit());
}

PLY_TEST_CASE("Float2 swizzles") {
    Float2 v = {4, 5};
    PLY_TEST_CHECK(v.swizzle(1, 0) == Float2{5, 4});
    PLY_TEST_CHECK(v.swizzle(1, 0, 1) == Float3{5, 4, 5});
    PLY_TEST_CHECK(v.swizzle(1, 1, 1, 0) == Float4{5, 5, 5, 4});
}

PLY_TEST_CASE("Float2 dot product") {
    Float2 v1 = {2, 3};
    Float2 v2 = {4, 5};
    PLY_TEST_CHECK(dot(v1, v2) == 23);
    PLY_TEST_CHECK(v1.length2() == dot(v1, v1));
}

PLY_TEST_CASE("Float2 cross product") {
    PLY_TEST_CHECK(cross(Float2{1, 0}, Float2{0, 1}) == 1.f);
    PLY_TEST_CHECK(cross(Float2{1, 0}, Float2{1, 0}) == 0.f);
    PLY_TEST_CHECK(cross(Float2{2, 2}, Float2{0, 4}) == 8);
    PLY_TEST_CHECK(cross(Float2{2, 2}, Float2{4, 0}) == -8);
}

PLY_TEST_CASE("Float2 clamp") {
    PLY_TEST_CHECK(clamp(Float2{2, 2}, 0, 1) == Float2{1, 1});
    PLY_TEST_CHECK(clamp(Float2{2, 0.5f}, 0, 1) == Float2{1, 0.5f});
    PLY_TEST_CHECK(clamp(Float2{-1, -1}, 0, 1) == Float2{0, 0});
    PLY_TEST_CHECK(clamp(Float2{3, 4}, Float2{0, 1}, Float2{1, 2}) == Float2{1, 2});
    PLY_TEST_CHECK(clamp(Float2{3, 1.5f}, Float2{0, 1}, Float2{1, 2}) == Float2{1, 1.5f});
    PLY_TEST_CHECK(clamp(Float2{-1, -1}, Float2{0, 1}, Float2{1, 2}) == Float2{0, 1});
}

PLY_TEST_CASE("Float2 abs") {
    PLY_TEST_CHECK(abs(Float2{1, 1}) == Float2{1, 1});
    PLY_TEST_CHECK(abs(Float2{-1, -1}) == Float2{1, 1});
    PLY_TEST_CHECK(abs(Float2{-2, 3}) == Float2{2, 3});
    PLY_TEST_CHECK(abs(Float2{2, -3}) == Float2{2, 3});
}

PLY_TEST_CASE("Float2 pow") {
    PLY_TEST_CHECK(pow(Float2{1, 2}, 2) == Float2{1, 4});
    PLY_TEST_CHECK(pow(2, Float2{1, 2}) == Float2{2, 4});
}

PLY_TEST_CASE("Float2 min") {
    PLY_TEST_CHECK(min(Float2{1, 0}, Float2{0, 1}) == Float2{0, 0});
    PLY_TEST_CHECK(min(Float2{0, 1}, Float2{1, 0}) == Float2{0, 0});
    PLY_TEST_CHECK(min(Float2{2, 2}, Float2{3, 3}) == Float2{2, 2});
    PLY_TEST_CHECK(min(Float2{3, 3}, Float2{2, 2}) == Float2{2, 2});
}

PLY_TEST_CASE("Float2 max") {
    PLY_TEST_CHECK(max(Float2{1, 0}, Float2{0, 1}) == Float2{1, 1});
    PLY_TEST_CHECK(max(Float2{0, 1}, Float2{1, 0}) == Float2{1, 1});
    PLY_TEST_CHECK(max(Float2{2, 2}, Float2{3, 3}) == Float2{3, 3});
    PLY_TEST_CHECK(max(Float2{3, 3}, Float2{2, 2}) == Float2{3, 3});
}

PLY_TEST_CASE("Float2 comparisons (all)") {
    PLY_TEST_CHECK(all(Float2{-1, -1} < 0));
    PLY_TEST_CHECK(!all(Float2{1, -1} <= 0));
    PLY_TEST_CHECK(!all(0 > Float2{-1, 1}));
    PLY_TEST_CHECK(!all(0 >= Float2{1, 1}));
    PLY_TEST_CHECK(!all(Float2{0, 0} < 0));
    PLY_TEST_CHECK(all(Float2{0, 0} <= 0));
    PLY_TEST_CHECK(all(Float2{1, 2} > Float2{0, 1}));
    PLY_TEST_CHECK(!all(Float2{1, 2} >= Float2{2, 1}));
    PLY_TEST_CHECK(!all(Float2{0, 3} < Float2{1, 2}));
    PLY_TEST_CHECK(!all(Float2{2, 3} <= Float2{1, 2}));
    PLY_TEST_CHECK(!all(Float2{1, 2} > Float2{1, 2}));
    PLY_TEST_CHECK(all(Float2{1, 2} >= Float2{1, 2}));
}

PLY_TEST_CASE("Float2 comparisons (any)") {
    PLY_TEST_CHECK(!any(Float2{-1, -1} >= 0));
    PLY_TEST_CHECK(any(Float2{1, -1} > 0));
    PLY_TEST_CHECK(any(0 <= Float2{-1, 1}));
    PLY_TEST_CHECK(any(0 < Float2{1, 1}));
    PLY_TEST_CHECK(any(Float2{0, 0} >= 0));
    PLY_TEST_CHECK(!any(Float2{0, 0} > 0));
    PLY_TEST_CHECK(!any(Float2{1, 2} <= Float2{0, 1}));
    PLY_TEST_CHECK(any(Float2{1, 2} < Float2{2, 1}));
    PLY_TEST_CHECK(any(Float2{0, 3} >= Float2{1, 2}));
    PLY_TEST_CHECK(any(Float2{2, 3} > Float2{1, 2}));
    PLY_TEST_CHECK(any(Float2{1, 2} <= Float2{1, 2}));
    PLY_TEST_CHECK(!any(Float2{1, 2} < Float2{1, 2}));
}

PLY_TEST_CASE("Float2 roundNearest") {
    PLY_TEST_CHECK(roundNearest(Float2{0.3f, 1.4f}, 2) == Float2{0, 2});
    PLY_TEST_CHECK(roundNearest(Float2{-0.3f, 1.4f}, 1) == Float2{0, 1});
    PLY_TEST_CHECK(roundNearest(Float2{0.3f, -1.4f}, 1) == Float2{0, -1});
    PLY_TEST_CHECK(roundNearest(Float2{-0.3f, 1.4f}, 0.5f) == Float2{-0.5f, 1.5f});
}

PLY_TEST_CASE("Float2 roundUp") {
    PLY_TEST_CHECK(roundUp(Float2{0.3f, 1.4f}, 2) == Float2{2, 2});
    PLY_TEST_CHECK(roundUp(Float2{-0.3f, 1.4f}, 1) == Float2{0, 2});
    PLY_TEST_CHECK(roundUp(Float2{0.3f, -1.4f}, 1) == Float2{1, -1});
    PLY_TEST_CHECK(roundUp(Float2{-0.3f, 1.4f}, 0.5f) == Float2{0, 1.5f});
}

PLY_TEST_CASE("Float2 roundDown") {
    PLY_TEST_CHECK(roundDown(Float2{0.3f, 1.4f}, 2) == Float2{0, 0});
    PLY_TEST_CHECK(roundDown(Float2{-0.3f, 1.4f}, 1) == Float2{-1, 1});
    PLY_TEST_CHECK(roundDown(Float2{0.3f, -1.4f}, 1) == Float2{0, -2});
    PLY_TEST_CHECK(roundDown(Float2{-0.3f, 1.4f}, 0.5f) == Float2{-0.5f, 1});
}

PLY_TEST_CASE("Float2 isRounded") {
    PLY_TEST_CHECK(isRounded(Float2{0, 1}, 1));
    PLY_TEST_CHECK(!isRounded(Float2{0.5f, 0}, 1));
    PLY_TEST_CHECK(!isRounded(Float2{1, -0.5f}, 1));
    PLY_TEST_CHECK(isRounded(Float2{0, 1}, 0.5f));
    PLY_TEST_CHECK(isRounded(Float2{0.5f, -0.f}, 0.5f));
    PLY_TEST_CHECK(isRounded(Float2{1, -0.5f}, 0.5f));
    PLY_TEST_CHECK(!isRounded(Float2{-0.5f, 0.3f}, 0.5f));
    PLY_TEST_CHECK(!isRounded(Float2{0, 1}, 2));
    PLY_TEST_CHECK(isRounded(Float2{4, 8}, 2));
}

} // namespace ply


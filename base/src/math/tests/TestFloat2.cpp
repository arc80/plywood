/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-math.h>
#include <ply-test/TestSuite.h>

namespace ply {

#define PLY_TEST_CASE_PREFIX Float2_

PLY_TEST_CASE("vec2 constructors") {
    vec2 v = {1};
    PLY_TEST_CHECK(v.x == 1 && v.y == 1);
    vec2 v2 = {1, 2};
    PLY_TEST_CHECK(v2.x == 1 && v2.y == 2);
    vec2 v3 = v2;
    PLY_TEST_CHECK(v3.x == 1 && v3.y == 2);
}

PLY_TEST_CASE("vec2 conversions") {
    auto v = (ivec2) vec2{1, 2};
    PLY_TEST_CHECK(v.x == 1 && v.y == 2);
    auto v2 = (TVec2<s16>) vec2{1, 2};
    PLY_TEST_CHECK(v2.x == 1 && v2.y == 2);
}

PLY_TEST_CASE("vec2 copy assignment") {
    vec2 v;
    v = vec2{1, 2};
    PLY_TEST_CHECK(v.x == 1 && v.y == 2);
}

PLY_TEST_CASE("vec2 comparisons") {
    PLY_TEST_CHECK(vec2{1, 2} == vec2{1, 2});
    PLY_TEST_CHECK(vec2{0, 0} == vec2{0, -0.f});
    PLY_TEST_CHECK(vec2{1, 2} != vec2{2, 1});
    PLY_TEST_CHECK(!(vec2{1, 2} != vec2{1, 2}));
    PLY_TEST_CHECK(!(vec2{0, 0} != vec2{0, -0.f}));
    PLY_TEST_CHECK(!(vec2{1, 2} == vec2{2, 1}));
}

PLY_TEST_CASE("vec2 is_within") {
    PLY_TEST_CHECK(is_within(vec2{4}, vec2{4}, 1e-6f));
    PLY_TEST_CHECK(is_within(vec2{5}, vec2{5}, 0));
    PLY_TEST_CHECK(!is_within(vec2{5}, vec2{6}, 0));
    PLY_TEST_CHECK(is_within(vec2{1, 0}, vec2{0.9999f, 0.0001f}, 1e-3f));
    PLY_TEST_CHECK(!is_within(vec2{1, 0}, vec2{0.999f, 0.001f}, 1e-3f));
}

PLY_TEST_CASE("vec2 unary negation") {
    PLY_TEST_CHECK(-vec2{1, 2} == vec2{-1, -2});
}

PLY_TEST_CASE("vec2 addition") {
    PLY_TEST_CHECK(vec2{1, 2} + vec2{3, 4} == vec2{4, 6});
    PLY_TEST_CHECK(vec2{1, 2} + 3 == vec2{4, 5});
    PLY_TEST_CHECK(3 + vec2{1, 2} == vec2{4, 5});
    vec2 v = {4, 5};
    v += vec2{1, 2};
    PLY_TEST_CHECK(v == vec2{5, 7});
    v += 1;
    PLY_TEST_CHECK(v == vec2{6, 8});
}

PLY_TEST_CASE("vec2 subtraction") {
    PLY_TEST_CHECK(vec2{3, 4} - vec2{1, 2} == vec2{2, 2});
    PLY_TEST_CHECK(vec2{2, 3} - 1 == vec2{1, 2});
    PLY_TEST_CHECK(6 - vec2{1, 2} == vec2{5, 4});
    vec2 v = {5, 4};
    v -= vec2{1, 2};
    PLY_TEST_CHECK(v.x == 4 && v.y == 2);
    v -= 1;
    PLY_TEST_CHECK(v.x == 3 && v.y == 1);
}

PLY_TEST_CASE("vec2 component-wise multiplication") {
    PLY_TEST_CHECK(vec2{1, 2} * vec2{3, 4} == vec2{3, 8});
    PLY_TEST_CHECK(vec2{1, 2} * 3 == vec2{3, 6});
    PLY_TEST_CHECK(3 * vec2{1, 2} == vec2{3, 6});
    vec2 v = {3, 6};
    v *= vec2{1, 2};
    PLY_TEST_CHECK(v.x == 3 && v.y == 12);
    v *= 2;
    PLY_TEST_CHECK(v.x == 6 && v.y == 24);
}

PLY_TEST_CASE("vec2 component-wise division") {
    PLY_TEST_CHECK(vec2{2, 6} / vec2{2, 3} == vec2{1, 2});
    PLY_TEST_CHECK(vec2{4, 6} / 2 == vec2{2, 3});
    PLY_TEST_CHECK(8 / vec2{4, 2} == vec2{2, 4});
    vec2 v = {2, 4};
    v /= vec2{1, 2};
    PLY_TEST_CHECK(v.x == 2 && v.y == 2);
    v /= 2;
    PLY_TEST_CHECK(v.x == 1 && v.y == 1);
}

PLY_TEST_CASE("vec2 lengths") {
    PLY_TEST_CHECK(square(vec2{3, 4}) == 25);
    PLY_TEST_CHECK(vec2{3, 4}.length() == 5);
    PLY_TEST_CHECK(vec2{1, 0}.is_unit());
    PLY_TEST_CHECK(!vec2{1, 1}.is_unit());
    PLY_TEST_CHECK(vec2{999, 666}.normalized().is_unit());
    PLY_TEST_CHECK(vec2{0}.safe_normalized().is_unit());
}

PLY_TEST_CASE("vec2 swizzles") {
    vec2 v = {4, 5};
    PLY_TEST_CHECK(v.swizzle(1, 0) == vec2{5, 4});
    PLY_TEST_CHECK(v.swizzle(1, 0, 1) == vec3{5, 4, 5});
    PLY_TEST_CHECK(v.swizzle(1, 1, 1, 0) == vec4{5, 5, 5, 4});
}

PLY_TEST_CASE("vec2 dot product") {
    vec2 v1 = {2, 3};
    vec2 v2 = {4, 5};
    PLY_TEST_CHECK(dot(v1, v2) == 23);
    PLY_TEST_CHECK(square(v1) == dot(v1, v1));
}

PLY_TEST_CASE("vec2 cross product") {
    PLY_TEST_CHECK(cross(vec2{1, 0}, vec2{0, 1}) == 1.f);
    PLY_TEST_CHECK(cross(vec2{1, 0}, vec2{1, 0}) == 0.f);
    PLY_TEST_CHECK(cross(vec2{2, 2}, vec2{0, 4}) == 8);
    PLY_TEST_CHECK(cross(vec2{2, 2}, vec2{4, 0}) == -8);
}

PLY_TEST_CASE("vec2 clamp") {
    PLY_TEST_CHECK(clamp(vec2{2, 2}, 0, 1) == vec2{1, 1});
    PLY_TEST_CHECK(clamp(vec2{2, 0.5f}, 0, 1) == vec2{1, 0.5f});
    PLY_TEST_CHECK(clamp(vec2{-1, -1}, 0, 1) == vec2{0, 0});
    PLY_TEST_CHECK(clamp(vec2{3, 4}, vec2{0, 1}, vec2{1, 2}) == vec2{1, 2});
    PLY_TEST_CHECK(clamp(vec2{3, 1.5f}, vec2{0, 1}, vec2{1, 2}) == vec2{1, 1.5f});
    PLY_TEST_CHECK(clamp(vec2{-1, -1}, vec2{0, 1}, vec2{1, 2}) == vec2{0, 1});
}

PLY_TEST_CASE("vec2 abs") {
    PLY_TEST_CHECK(abs(vec2{1, 1}) == vec2{1, 1});
    PLY_TEST_CHECK(abs(vec2{-1, -1}) == vec2{1, 1});
    PLY_TEST_CHECK(abs(vec2{-2, 3}) == vec2{2, 3});
    PLY_TEST_CHECK(abs(vec2{2, -3}) == vec2{2, 3});
}

PLY_TEST_CASE("vec2 pow") {
    PLY_TEST_CHECK(pow(vec2{1, 2}, 2) == vec2{1, 4});
    PLY_TEST_CHECK(pow(2, vec2{1, 2}) == vec2{2, 4});
}

PLY_TEST_CASE("vec2 min") {
    PLY_TEST_CHECK(min(vec2{1, 0}, vec2{0, 1}) == vec2{0, 0});
    PLY_TEST_CHECK(min(vec2{0, 1}, vec2{1, 0}) == vec2{0, 0});
    PLY_TEST_CHECK(min(vec2{2, 2}, vec2{3, 3}) == vec2{2, 2});
    PLY_TEST_CHECK(min(vec2{3, 3}, vec2{2, 2}) == vec2{2, 2});
}

PLY_TEST_CASE("vec2 max") {
    PLY_TEST_CHECK(max(vec2{1, 0}, vec2{0, 1}) == vec2{1, 1});
    PLY_TEST_CHECK(max(vec2{0, 1}, vec2{1, 0}) == vec2{1, 1});
    PLY_TEST_CHECK(max(vec2{2, 2}, vec2{3, 3}) == vec2{3, 3});
    PLY_TEST_CHECK(max(vec2{3, 3}, vec2{2, 2}) == vec2{3, 3});
}

PLY_TEST_CASE("vec2 comparisons (all)") {
    PLY_TEST_CHECK(all(vec2{-1, -1} < 0));
    PLY_TEST_CHECK(!all(vec2{1, -1} <= 0));
    PLY_TEST_CHECK(!all(0 > vec2{-1, 1}));
    PLY_TEST_CHECK(!all(0 >= vec2{1, 1}));
    PLY_TEST_CHECK(!all(vec2{0, 0} < 0));
    PLY_TEST_CHECK(all(vec2{0, 0} <= 0));
    PLY_TEST_CHECK(all(vec2{1, 2} > vec2{0, 1}));
    PLY_TEST_CHECK(!all(vec2{1, 2} >= vec2{2, 1}));
    PLY_TEST_CHECK(!all(vec2{0, 3} < vec2{1, 2}));
    PLY_TEST_CHECK(!all(vec2{2, 3} <= vec2{1, 2}));
    PLY_TEST_CHECK(!all(vec2{1, 2} > vec2{1, 2}));
    PLY_TEST_CHECK(all(vec2{1, 2} >= vec2{1, 2}));
}

PLY_TEST_CASE("vec2 comparisons (any)") {
    PLY_TEST_CHECK(!any(vec2{-1, -1} >= 0));
    PLY_TEST_CHECK(any(vec2{1, -1} > 0));
    PLY_TEST_CHECK(any(0 <= vec2{-1, 1}));
    PLY_TEST_CHECK(any(0 < vec2{1, 1}));
    PLY_TEST_CHECK(any(vec2{0, 0} >= 0));
    PLY_TEST_CHECK(!any(vec2{0, 0} > 0));
    PLY_TEST_CHECK(!any(vec2{1, 2} <= vec2{0, 1}));
    PLY_TEST_CHECK(any(vec2{1, 2} < vec2{2, 1}));
    PLY_TEST_CHECK(any(vec2{0, 3} >= vec2{1, 2}));
    PLY_TEST_CHECK(any(vec2{2, 3} > vec2{1, 2}));
    PLY_TEST_CHECK(any(vec2{1, 2} <= vec2{1, 2}));
    PLY_TEST_CHECK(!any(vec2{1, 2} < vec2{1, 2}));
}

PLY_TEST_CASE("vec2 roundNearest") {
    PLY_TEST_CHECK(round_nearest(vec2{0.3f, 1.4f}, 2) == vec2{0, 2});
    PLY_TEST_CHECK(round_nearest(vec2{-0.3f, 1.4f}, 1) == vec2{0, 1});
    PLY_TEST_CHECK(round_nearest(vec2{0.3f, -1.4f}, 1) == vec2{0, -1});
    PLY_TEST_CHECK(round_nearest(vec2{-0.3f, 1.4f}, 0.5f) == vec2{-0.5f, 1.5f});
}

PLY_TEST_CASE("vec2 roundUp") {
    PLY_TEST_CHECK(round_up(vec2{0.3f, 1.4f}, 2) == vec2{2, 2});
    PLY_TEST_CHECK(round_up(vec2{-0.3f, 1.4f}, 1) == vec2{0, 2});
    PLY_TEST_CHECK(round_up(vec2{0.3f, -1.4f}, 1) == vec2{1, -1});
    PLY_TEST_CHECK(round_up(vec2{-0.3f, 1.4f}, 0.5f) == vec2{0, 1.5f});
}

PLY_TEST_CASE("vec2 roundDown") {
    PLY_TEST_CHECK(round_down(vec2{0.3f, 1.4f}, 2) == vec2{0, 0});
    PLY_TEST_CHECK(round_down(vec2{-0.3f, 1.4f}, 1) == vec2{-1, 1});
    PLY_TEST_CHECK(round_down(vec2{0.3f, -1.4f}, 1) == vec2{0, -2});
    PLY_TEST_CHECK(round_down(vec2{-0.3f, 1.4f}, 0.5f) == vec2{-0.5f, 1});
}

PLY_TEST_CASE("vec2 isRounded") {
    PLY_TEST_CHECK(is_rounded(vec2{0, 1}, 1));
    PLY_TEST_CHECK(!is_rounded(vec2{0.5f, 0}, 1));
    PLY_TEST_CHECK(!is_rounded(vec2{1, -0.5f}, 1));
    PLY_TEST_CHECK(is_rounded(vec2{0, 1}, 0.5f));
    PLY_TEST_CHECK(is_rounded(vec2{0.5f, -0.f}, 0.5f));
    PLY_TEST_CHECK(is_rounded(vec2{1, -0.5f}, 0.5f));
    PLY_TEST_CHECK(!is_rounded(vec2{-0.5f, 0.3f}, 0.5f));
    PLY_TEST_CHECK(!is_rounded(vec2{0, 1}, 2));
    PLY_TEST_CHECK(is_rounded(vec2{4, 8}, 2));
}

} // namespace ply

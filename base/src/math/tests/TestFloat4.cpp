/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-math.h>
#include <ply-test/TestSuite.h>

namespace ply {

#define PLY_TEST_CASE_PREFIX Float4_

PLY_TEST_CASE("vec4 constructors") {
    vec4 v = {1};
    PLY_TEST_CHECK(v.x == 1 && v.y == 1 && v.z == 1 && v.w == 1);
    vec4 v2 = {1, 2, 5, 0};
    PLY_TEST_CHECK(v2.x == 1 && v2.y == 2 && v2.z == 5 && v2.w == 0);
    vec4 v3 = v2;
    PLY_TEST_CHECK(v3.x == 1 && v3.y == 2 && v3.z == 5 && v3.w == 0);
}

PLY_TEST_CASE("vec4 conversions") {
    auto v = (ivec4) vec4{1, 2, 5, 0};
    PLY_TEST_CHECK(v.x == 1 && v.y == 2 && v.z == 5 && v.w == 0);
    auto v2 = (TVec4<s16>) vec4{1, 2, 5, 0};
    PLY_TEST_CHECK(v2.x == 1 && v2.y == 2 && v2.z == 5 && v2.w == 0);
}

PLY_TEST_CASE("vec4 copy assignment") {
    vec4 v;
    v = vec4{1, 2, 5, 0};
    PLY_TEST_CHECK(v.x == 1 && v.y == 2 && v.z == 5 && v.w == 0);
}

PLY_TEST_CASE("vec4 comparisons") {
    PLY_TEST_CHECK(vec4{1, 2, 5, 0} == vec4{1, 2, 5, 0});
    PLY_TEST_CHECK(vec4{0, 0, 0, -0.f} == vec4{0, -0.f, 0, 0});
    PLY_TEST_CHECK(vec4{1, 2, 5, 0} != vec4{2, 1, 5, 0});
    PLY_TEST_CHECK(!(vec4{1, 2, 5, 0} != vec4{1, 2, 5, 0}));
    PLY_TEST_CHECK(!(vec4{0, 0, 0, -0.f} != vec4{0, -0.f, 0, 0}));
    PLY_TEST_CHECK(!(vec4{1, 2, 5, 0} == vec4{2, 1, 5, 0}));
}

PLY_TEST_CASE("vec4 isNear") {
    PLY_TEST_CHECK(is_within(vec4{4}, vec4{4}, 1e-6f));
    PLY_TEST_CHECK(is_within(vec4{5}, vec4{5}, 0));
    PLY_TEST_CHECK(!is_within(vec4{5}, vec4{6}, 0));
    PLY_TEST_CHECK(
        is_within(vec4{1, 0, 2, 3}, vec4{0.9999f, 0.0001f, 1.9999f, 3.0001f}, 1e-3f));
    PLY_TEST_CHECK(
        !is_within(vec4{1, 0, 2, 3}, vec4{0.999f, 0.001f, 1.9999f, 3.0001f}, 1e-3f));
}

PLY_TEST_CASE("vec4 unary negation") {
    PLY_TEST_CHECK(-vec4{1, 2, 5, 0} == vec4{-1, -2, -5, 0});
}

PLY_TEST_CASE("vec4 addition") {
    PLY_TEST_CHECK(vec4{1, 2, 5, 2} + vec4{3, 4, 0, 1} == vec4{4, 6, 5, 3});
    PLY_TEST_CHECK(vec4{1, 2, 5, 2} + 3 == vec4{4, 5, 8, 5});
    PLY_TEST_CHECK(3 + vec4{1, 2, 5, 2} == vec4{4, 5, 8, 5});
    vec4 v = {4, 5, 8, 5};
    v += vec4{1, 2, 5, 1};
    PLY_TEST_CHECK(v == vec4{5, 7, 13, 6});
    v += 1;
    PLY_TEST_CHECK(v == vec4{6, 8, 14, 7});
}

PLY_TEST_CASE("vec4 subtraction") {
    PLY_TEST_CHECK(vec4{3, 4, 5, 7} - vec4{1, 2, 1, 2} == vec4{2, 2, 4, 5});
    PLY_TEST_CHECK(vec4{2, 3, 5, 7} - 1 == vec4{1, 2, 4, 6});
    PLY_TEST_CHECK(6 - vec4{1, 2, 1, 0} == vec4{5, 4, 5, 6});
    vec4 v = {5, 4, 5, 6};
    v -= vec4{1, 2, 0, 2};
    PLY_TEST_CHECK(v == vec4{4, 2, 5, 4});
    v -= 1;
    PLY_TEST_CHECK(v == vec4{3, 1, 4, 3});
}

PLY_TEST_CASE("vec4 component-wise multiplication") {
    PLY_TEST_CHECK(vec4{1, 2, 5, 0} * vec4{3, 4, 2, 5} == vec4{3, 8, 10, 0});
    PLY_TEST_CHECK(vec4{1, 2, 5, 0} * 3 == vec4{3, 6, 15, 0});
    PLY_TEST_CHECK(3 * vec4{1, 2, 5, 0} == vec4{3, 6, 15, 0});
    vec4 v = {3, 6, 15, 0};
    v *= vec4{1, 2, 3, 7};
    PLY_TEST_CHECK(v == vec4{3, 12, 45, 0});
    v *= 2;
    PLY_TEST_CHECK(v == vec4{6, 24, 90, 0});
}

PLY_TEST_CASE("vec4 component-wise division") {
    PLY_TEST_CHECK(vec4{2, 6, 4, 9} / vec4{2, 3, 1, 3} == vec4{1, 2, 4, 3});
    PLY_TEST_CHECK(vec4{4, 6, 2, 0} / 2 == vec4{2, 3, 1, 0});
    PLY_TEST_CHECK(8 / vec4{4, 2, 1, 8} == vec4{2, 4, 8, 1});
    vec4 v = {2, 4, 8, 1};
    v /= vec4{1, 2, 1, 1};
    PLY_TEST_CHECK(v == vec4{2, 2, 8, 1});
    v /= 2;
    PLY_TEST_CHECK(v == vec4{1, 1, 4, 0.5f});
}

PLY_TEST_CASE("vec4 lengths") {
    PLY_TEST_CHECK(square(vec4{3, 4, 2, -1}) == 30);
    PLY_TEST_CHECK((vec4{3, 4, 2, -1}.length() - 5.477225f) < 1e-3f);
    PLY_TEST_CHECK(vec4{1, 0, 0, 0}.is_unit());
    PLY_TEST_CHECK(!vec4{1, 1, 1, 1}.is_unit());
    PLY_TEST_CHECK(vec4{999, 666, 333, -444}.normalized().is_unit());
    PLY_TEST_CHECK(vec4{0}.safe_normalized().is_unit());
}

PLY_TEST_CASE("vec4 swizzles") {
    vec4 v = {4, 5, 6, 7};
    PLY_TEST_CHECK(v.swizzle(3, 1) == vec2{7, 5});
    PLY_TEST_CHECK(v.swizzle(2, 0, 3) == vec3{6, 4, 7});
    PLY_TEST_CHECK(v.swizzle(1, 2, 3, 0) == vec4{5, 6, 7, 4});
}

PLY_TEST_CASE("vec4 dot product") {
    vec4 v1 = {2, 3, 1, 3};
    vec4 v2 = {4, 5, 1, 0};
    PLY_TEST_CHECK(dot(v1, v2) == 24);
    PLY_TEST_CHECK(square(v1) == dot(v1, v1));
}

PLY_TEST_CASE("vec4 clamp") {
    PLY_TEST_CHECK(clamp(vec4{2, 2, 2, 2}, 0, 1) == vec4{1, 1, 1, 1});
    PLY_TEST_CHECK(clamp(vec4{2, 0.5f, 0, 1.5f}, 0, 1) == vec4{1, 0.5f, 0, 1});
    PLY_TEST_CHECK(clamp(vec4{-1, -1, -1, -1}, 0, 1) == vec4{0, 0, 0, 0});
    PLY_TEST_CHECK(clamp(vec4{3, 4, 5, 6}, vec4{0, 1, 2, 3}, vec4{1, 2, 3, 4}) ==
                   vec4{1, 2, 3, 4});
    PLY_TEST_CHECK(clamp(vec4{3, 1.5f, 1, 3.5f}, vec4{0, 1, 2, 3}, vec4{1, 2, 3, 4}) ==
                   vec4{1, 1.5f, 2, 3.5f});
    PLY_TEST_CHECK(clamp(vec4{-1, -1, -1, -1}, vec4{0, 1, 2, 3}, vec4{1, 2, 3, 4}) ==
                   vec4{0, 1, 2, 3});
}

PLY_TEST_CASE("vec4 abs") {
    PLY_TEST_CHECK(abs(vec4{1, 1, 1, 1}) == vec4{1, 1, 1, 1});
    PLY_TEST_CHECK(abs(vec4{-1, -1, -1, -1}) == vec4{1, 1, 1, 1});
    PLY_TEST_CHECK(abs(vec4{-2, 3, 0, -1}) == vec4{2, 3, 0, 1});
    PLY_TEST_CHECK(abs(vec4{0, 2, -3, 1}) == vec4{0, 2, 3, 1});
}

PLY_TEST_CASE("vec4 min") {
    PLY_TEST_CHECK(min(vec4{1, 0, 1, 0}, vec4{0, 1, 0, 1}) == vec4{0, 0, 0, 0});
    PLY_TEST_CHECK(min(vec4{0, 1, 0, 1}, vec4{1, 0, 1, 0}) == vec4{0, 0, 0, 0});
    PLY_TEST_CHECK(min(vec4{2, 2, 2, 2}, vec4{3, 3, 3, 3}) == vec4{2, 2, 2, 2});
    PLY_TEST_CHECK(min(vec4{3, 3, 3, 3}, vec4{2, 2, 2, 2}) == vec4{2, 2, 2, 2});
}

PLY_TEST_CASE("vec4 max") {
    PLY_TEST_CHECK(max(vec4{1, 0, 1, 0}, vec4{0, 1, 0, 1}) == vec4{1, 1, 1, 1});
    PLY_TEST_CHECK(max(vec4{0, 1, 0, 1}, vec4{1, 0, 1, 0}) == vec4{1, 1, 1, 1});
    PLY_TEST_CHECK(max(vec4{2, 2, 2, 2}, vec4{3, 3, 3, 3}) == vec4{3, 3, 3, 3});
    PLY_TEST_CHECK(max(vec4{3, 3, 3, 3}, vec4{2, 2, 2, 2}) == vec4{3, 3, 3, 3});
}

PLY_TEST_CASE("vec4 comparisons (all)") {
    PLY_TEST_CHECK(all(vec4{-1, -1, -1, -1} < 0));
    PLY_TEST_CHECK(!all(vec4{1, -1, 1, -1} <= 0));
    PLY_TEST_CHECK(!all(0 > vec4{-1, 1, -1, 1}));
    PLY_TEST_CHECK(!all(0 >= vec4{1, 1, 1, 1}));
    PLY_TEST_CHECK(!all(vec4{0, 0, 0, 0} < 0));
    PLY_TEST_CHECK(all(vec4{0, 0, 0, 0} <= 0));
    PLY_TEST_CHECK(all(vec4{1, 2, 4, 3} > vec4{0, 1, 3, 2}));
    PLY_TEST_CHECK(!all(vec4{1, 2, 1, 3} >= vec4{2, 1, 0, 2}));
    PLY_TEST_CHECK(!all(vec4{0, 3, 3, 2} < vec4{1, 4, 2, 3}));
    PLY_TEST_CHECK(!all(vec4{3, 2, 3, 2} <= vec4{4, 1, 2, 3}));
    PLY_TEST_CHECK(!all(vec4{3, 1, 2, 0} > vec4{3, 1, 2, 0}));
    PLY_TEST_CHECK(all(vec4{3, 1, 2, 0} >= vec4{3, 1, 2, 0}));
}

PLY_TEST_CASE("vec4 comparisons (all)") {
    PLY_TEST_CHECK(!any(vec4{-1, -1, -1, -1} >= 0));
    PLY_TEST_CHECK(any(vec4{1, -1, 1, -1} > 0));
    PLY_TEST_CHECK(any(0 <= vec4{-1, 1, -1, 1}));
    PLY_TEST_CHECK(any(0 < vec4{1, 1, 1, 1}));
    PLY_TEST_CHECK(any(vec4{0, 0, 0, 0} >= 0));
    PLY_TEST_CHECK(!any(vec4{0, 0, 0, 0} > 0));
    PLY_TEST_CHECK(!any(vec4{1, 2, 4, 3} <= vec4{0, 1, 3, 2}));
    PLY_TEST_CHECK(any(vec4{1, 2, 1, 3} < vec4{2, 1, 0, 2}));
    PLY_TEST_CHECK(any(vec4{0, 3, 3, 2} >= vec4{1, 4, 2, 3}));
    PLY_TEST_CHECK(any(vec4{3, 2, 3, 2} > vec4{4, 1, 2, 3}));
    PLY_TEST_CHECK(any(vec4{3, 1, 2, 0} <= vec4{3, 1, 2, 0}));
    PLY_TEST_CHECK(!any(vec4{3, 1, 2, 0} < vec4{3, 1, 2, 0}));
}

PLY_TEST_CASE("vec4 roundNearest") {
    PLY_TEST_CHECK(round_nearest(vec4{0.3f, 1.4f, 0.8f, 1.2f}, 2) == vec4{0, 2, 0, 2});
    PLY_TEST_CHECK(round_nearest(vec4{-0.3f, 1.4f, 0.8f, -1.2f}, 1) ==
                   vec4{0, 1, 1, -1});
    PLY_TEST_CHECK(round_nearest(vec4{0.3f, -1.4f, -0.8f, 1.2f}, 1) ==
                   vec4{0, -1, -1, 1});
    PLY_TEST_CHECK(round_nearest(vec4{-0.3f, 1.4f, 0.8f, -1.2f}, 0.5f) ==
                   vec4{-0.5f, 1.5f, 1, -1});
}

PLY_TEST_CASE("vec4 roundUp") {
    PLY_TEST_CHECK(round_up(vec4{0.3f, 1.4f, 0.8f, 1.2f}, 2) == vec4{2, 2, 2, 2});
    PLY_TEST_CHECK(round_up(vec4{-0.3f, 1.4f, 0.8f, -1.2f}, 1) == vec4{0, 2, 1, -1});
    PLY_TEST_CHECK(round_up(vec4{0.3f, -1.4f, -0.8f, 1.2f}, 1) == vec4{1, -1, 0, 2});
    PLY_TEST_CHECK(round_up(vec4{-0.3f, 1.4f, 0.8f, -1.2f}, 0.5f) ==
                   vec4{0, 1.5f, 1, -1});
}

PLY_TEST_CASE("vec4 roundDown") {
    PLY_TEST_CHECK(round_down(vec4{0.3f, 1.4f, 0.8f, 1.2f}, 2) == vec4{0, 0, 0, 0});
    PLY_TEST_CHECK(round_down(vec4{-0.3f, 1.4f, 0.8f, -1.2f}, 1) == vec4{-1, 1, 0, -2});
    PLY_TEST_CHECK(round_down(vec4{0.3f, -1.4f, -0.8f, 1.2f}, 1) == vec4{0, -2, -1, 1});
    PLY_TEST_CHECK(round_down(vec4{-0.3f, 1.4f, 0.8f, -1.2f}, 0.5f) ==
                   vec4{-0.5f, 1, 0.5f, -1.5f});
}

PLY_TEST_CASE("vec4 isRounded") {
    PLY_TEST_CHECK(is_rounded(vec4{0, 1, 2, 1}, 1));
    PLY_TEST_CHECK(!is_rounded(vec4{0.5f, 0, 2, 0}, 1));
    PLY_TEST_CHECK(!is_rounded(vec4{1, -0.5f, 0, -0.5f}, 1));
    PLY_TEST_CHECK(!is_rounded(vec4{0, 1, 0.5f, 1}, 1));
    PLY_TEST_CHECK(is_rounded(vec4{0, 1, 2, 1}, 0.5f));
    PLY_TEST_CHECK(is_rounded(vec4{0.5f, -0.f, -1.5f, -0.f}, 0.5f));
    PLY_TEST_CHECK(is_rounded(vec4{1, -0.5f, 0.f, -0.5f}, 0.5f));
    PLY_TEST_CHECK(!is_rounded(vec4{-0.5f, 0.3f, 0.f, 0.3f}, 0.5f));
    PLY_TEST_CHECK(!is_rounded(vec4{0, 1, 2, 1}, 2));
    PLY_TEST_CHECK(is_rounded(vec4{4, 8, -2, 8}, 2));
}

} // namespace ply

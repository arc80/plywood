/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-math.h>
#include <ply-test/TestSuite.h>

namespace ply {

#define PLY_TEST_CASE_PREFIX Float3_

PLY_TEST_CASE("vec3 constructors") {
    vec3 v = {1};
    PLY_TEST_CHECK(v.x == 1 && v.y == 1 && v.z == 1);
    vec3 v2 = {1, 2, 5};
    PLY_TEST_CHECK(v2.x == 1 && v2.y == 2 && v2.z == 5);
    vec3 v3 = v2;
    PLY_TEST_CHECK(v3.x == 1 && v3.y == 2 && v3.z == 5);
}

PLY_TEST_CASE("vec3 conversions") {
    auto v = (ivec3) vec3{1, 2, 5};
    PLY_TEST_CHECK(v.x == 1 && v.y == 2 && v.z == 5);
    auto v2 = (TVec3<s16>) vec3{1, 2, 5};
    PLY_TEST_CHECK(v2.x == 1 && v2.y == 2 && v2.z == 5);
}

PLY_TEST_CASE("vec3 copy assignment") {
    vec3 v;
    v = vec3{1, 2, 5};
    PLY_TEST_CHECK(v.x == 1 && v.y == 2 && v.z == 5);
}

PLY_TEST_CASE("vec3 comparisons") {
    PLY_TEST_CHECK(vec3{1, 2, 5} == vec3{1, 2, 5});
    PLY_TEST_CHECK(vec3{0, 0, 0} == vec3{0, -0.f, 0});
    PLY_TEST_CHECK(vec3{1, 2, 5} != vec3{2, 1, 5});
    PLY_TEST_CHECK(!(vec3{1, 2, 5} != vec3{1, 2, 5}));
    PLY_TEST_CHECK(!(vec3{0, 0, 0} != vec3{0, -0.f, 0}));
    PLY_TEST_CHECK(!(vec3{1, 2, 5} == vec3{2, 1, 5}));
}

PLY_TEST_CASE("vec3 isNear") {
    PLY_TEST_CHECK(is_within(vec3{4}, vec3{4}, 1e-6f));
    PLY_TEST_CHECK(is_within(vec3{5}, vec3{5}, 0));
    PLY_TEST_CHECK(!is_within(vec3{5}, vec3{6}, 0));
    PLY_TEST_CHECK(is_within(vec3{1, 0, 2}, vec3{0.9999f, 0.0001f, 1.9999f}, 1e-3f));
    PLY_TEST_CHECK(!is_within(vec3{1, 0, 2}, vec3{0.999f, 0.001f, 1.9999f}, 1e-3f));
}

PLY_TEST_CASE("vec3 unary negation") {
    PLY_TEST_CHECK(-vec3{1, 2, 5} == vec3{-1, -2, -5});
}

PLY_TEST_CASE("vec3 addition") {
    PLY_TEST_CHECK(vec3{1, 2, 5} + vec3{3, 4, 0} == vec3{4, 6, 5});
    PLY_TEST_CHECK(vec3{1, 2, 5} + 3 == vec3{4, 5, 8});
    PLY_TEST_CHECK(3 + vec3{1, 2, 5} == vec3{4, 5, 8});
    vec3 v = {4, 5, 8};
    v += vec3{1, 2, 5};
    PLY_TEST_CHECK(v == vec3{5, 7, 13});
    v += 1;
    PLY_TEST_CHECK(v == vec3{6, 8, 14});
}

PLY_TEST_CASE("vec3 subtraction") {
    PLY_TEST_CHECK(vec3{3, 4, 5} - vec3{1, 2, 1} == vec3{2, 2, 4});
    PLY_TEST_CHECK(vec3{2, 3, 5} - 1 == vec3{1, 2, 4});
    PLY_TEST_CHECK(6 - vec3{1, 2, 1} == vec3{5, 4, 5});
    vec3 v = {5, 4, 5};
    v -= vec3{1, 2, 0};
    PLY_TEST_CHECK(v == vec3{4, 2, 5});
    v -= 1;
    PLY_TEST_CHECK(v == vec3{3, 1, 4});
}

PLY_TEST_CASE("vec3 component-wise multiplication") {
    PLY_TEST_CHECK(vec3{1, 2, 5} * vec3{3, 4, 2} == vec3{3, 8, 10});
    PLY_TEST_CHECK(vec3{1, 2, 5} * 3 == vec3{3, 6, 15});
    PLY_TEST_CHECK(3 * vec3{1, 2, 5} == vec3{3, 6, 15});
    vec3 v = {3, 6, 15};
    v *= vec3{1, 2, 3};
    PLY_TEST_CHECK(v == vec3{3, 12, 45});
    v *= 2;
    PLY_TEST_CHECK(v == vec3{6, 24, 90});
}

PLY_TEST_CASE("vec3 component-wise division") {
    PLY_TEST_CHECK(vec3{2, 6, 4} / vec3{2, 3, 1} == vec3{1, 2, 4});
    PLY_TEST_CHECK(vec3{4, 6, 2} / 2 == vec3{2, 3, 1});
    PLY_TEST_CHECK(8 / vec3{4, 2, 1} == vec3{2, 4, 8});
    vec3 v = {2, 4, 8};
    v /= vec3{1, 2, 1};
    PLY_TEST_CHECK(v == vec3{2, 2, 8});
    v /= 2;
    PLY_TEST_CHECK(v == vec3{1, 1, 4});
}

PLY_TEST_CASE("vec3 lengths") {
    PLY_TEST_CHECK(square(vec3{3, 4, 2}) == 29);
    PLY_TEST_CHECK((vec3{3, 4, 2}.length() - 5.385164f) < 1e-3f);
    PLY_TEST_CHECK(vec3{1, 0, 0}.is_unit());
    PLY_TEST_CHECK(!vec3{1, 1, 1}.is_unit());
    PLY_TEST_CHECK(vec3{999, 666, 333}.normalized().is_unit());
    PLY_TEST_CHECK(vec3{0}.safe_normalized().is_unit());
}

PLY_TEST_CASE("vec3 swizzles") {
    vec3 v = {4, 5, 6};
    PLY_TEST_CHECK(v.swizzle(2, 1) == vec2{6, 5});
    PLY_TEST_CHECK(v.swizzle(2, 0, 1) == vec3{6, 4, 5});
    PLY_TEST_CHECK(v.swizzle(1, 2, 1, 0) == vec4{5, 6, 5, 4});
}

PLY_TEST_CASE("vec3 dot product") {
    vec3 v1 = {2, 3, 1};
    vec3 v2 = {4, 5, 1};
    PLY_TEST_CHECK(dot(v1, v2) == 24);
    PLY_TEST_CHECK(square(v1) == dot(v1, v1));
}

PLY_TEST_CASE("vec3 cross product") {
    PLY_TEST_CHECK(cross(vec3{1, 0, 0}, vec3{0, 1, 0}) == vec3{0, 0, 1});
    PLY_TEST_CHECK(cross(vec3{1, 0, 0}, vec3{0, 0, 1}) == vec3{0, -1, 0});
    PLY_TEST_CHECK(cross(vec3{1, 0, 0}, vec3{1, 0, 0}) == 0);
    PLY_TEST_CHECK(cross(vec3{2, 2, 1}, vec3{0, 4, 1}) == vec3{-2, -2, 8});
    PLY_TEST_CHECK(cross(vec3{2, 2, 2}, vec3{4, 0, 0}) == vec3{0, 8, -8});
}

PLY_TEST_CASE("vec3 clamp") {
    PLY_TEST_CHECK(clamp(vec3{2, 2, 2}, 0, 1) == vec3{1, 1, 1});
    PLY_TEST_CHECK(clamp(vec3{2, 0.5f, 0}, 0, 1) == vec3{1, 0.5f, 0});
    PLY_TEST_CHECK(clamp(vec3{-1, -1, -1}, 0, 1) == vec3{0, 0, 0});
    PLY_TEST_CHECK(clamp(vec3{3, 4, 5}, vec3{0, 1, 2}, vec3{1, 2, 3}) == vec3{1, 2, 3});
    PLY_TEST_CHECK(clamp(vec3{3, 1.5f, 1}, vec3{0, 1, 2}, vec3{1, 2, 3}) ==
                   vec3{1, 1.5f, 2});
    PLY_TEST_CHECK(clamp(vec3{-1, -1, -1}, vec3{0, 1, 2}, vec3{1, 2, 3}) ==
                   vec3{0, 1, 2});
}

PLY_TEST_CASE("vec3 abs") {
    PLY_TEST_CHECK(abs(vec3{1, 1, 1}) == vec3{1, 1, 1});
    PLY_TEST_CHECK(abs(vec3{-1, -1, -1}) == vec3{1, 1, 1});
    PLY_TEST_CHECK(abs(vec3{-2, 3, 0}) == vec3{2, 3, 0});
    PLY_TEST_CHECK(abs(vec3{0, 2, -3}) == vec3{0, 2, 3});
}

PLY_TEST_CASE("vec3 pow") {
    PLY_TEST_CHECK(pow(vec3{1, 2, 1}, 2) == vec3{1, 4, 1});
    PLY_TEST_CHECK(pow(2, vec3{1, 2, 0}) == vec3{2, 4, 1});
}

PLY_TEST_CASE("vec3 min") {
    PLY_TEST_CHECK(min(vec3{1, 0, 1}, vec3{0, 1, 0}) == vec3{0, 0, 0});
    PLY_TEST_CHECK(min(vec3{0, 1, 0}, vec3{1, 0, 1}) == vec3{0, 0, 0});
    PLY_TEST_CHECK(min(vec3{2, 2, 2}, vec3{3, 3, 3}) == vec3{2, 2, 2});
    PLY_TEST_CHECK(min(vec3{3, 3, 3}, vec3{2, 2, 2}) == vec3{2, 2, 2});
}

PLY_TEST_CASE("vec3 max") {
    PLY_TEST_CHECK(max(vec3{1, 0, 1}, vec3{0, 1, 0}) == vec3{1, 1, 1});
    PLY_TEST_CHECK(max(vec3{0, 1, 0}, vec3{1, 0, 1}) == vec3{1, 1, 1});
    PLY_TEST_CHECK(max(vec3{2, 2, 2}, vec3{3, 3, 3}) == vec3{3, 3, 3});
    PLY_TEST_CHECK(max(vec3{3, 3, 3}, vec3{2, 2, 2}) == vec3{3, 3, 3});
}

PLY_TEST_CASE("vec3 comparisons (all)") {
    PLY_TEST_CHECK(all(vec3{-1, -1, -1} < 0));
    PLY_TEST_CHECK(!all(vec3{1, -1, 1} <= 0));
    PLY_TEST_CHECK(!all(0 > vec3{-1, 1, -1}));
    PLY_TEST_CHECK(!all(0 >= vec3{1, 1, 1}));
    PLY_TEST_CHECK(!all(vec3{0, 0, 0} < 0));
    PLY_TEST_CHECK(all(vec3{0, 0, 0} <= 0));
    PLY_TEST_CHECK(all(vec3{1, 2, 4} > vec3{0, 1, 3}));
    PLY_TEST_CHECK(!all(vec3{1, 2, 1} >= vec3{2, 1, 0}));
    PLY_TEST_CHECK(!all(vec3{0, 3, 3} < vec3{1, 4, 2}));
    PLY_TEST_CHECK(!all(vec3{3, 2, 3} <= vec3{4, 1, 2}));
    PLY_TEST_CHECK(!all(vec3{3, 1, 2} > vec3{3, 1, 2}));
    PLY_TEST_CHECK(all(vec3{3, 1, 2} >= vec3{3, 1, 2}));
}

PLY_TEST_CASE("vec3 comparisons (all)") {
    PLY_TEST_CHECK(!any(vec3{-1, -1, -1} >= 0));
    PLY_TEST_CHECK(any(vec3{1, -1, 1} > 0));
    PLY_TEST_CHECK(any(0 <= vec3{-1, 1, -1}));
    PLY_TEST_CHECK(any(0 < vec3{1, 1, 1}));
    PLY_TEST_CHECK(any(vec3{0, 0, 0} >= 0));
    PLY_TEST_CHECK(!any(vec3{0, 0, 0} > 0));
    PLY_TEST_CHECK(!any(vec3{1, 2, 4} <= vec3{0, 1, 3}));
    PLY_TEST_CHECK(any(vec3{1, 2, 1} < vec3{2, 1, 0}));
    PLY_TEST_CHECK(any(vec3{0, 3, 3} >= vec3{1, 4, 2}));
    PLY_TEST_CHECK(any(vec3{3, 2, 3} > vec3{4, 1, 2}));
    PLY_TEST_CHECK(any(vec3{3, 1, 2} <= vec3{3, 1, 2}));
    PLY_TEST_CHECK(!any(vec3{3, 1, 2} < vec3{3, 1, 2}));
}

PLY_TEST_CASE("vec3 roundNearest") {
    PLY_TEST_CHECK(round_nearest(vec3{0.3f, 1.4f, 0.8f}, 2) == vec3{0, 2, 0});
    PLY_TEST_CHECK(round_nearest(vec3{-0.3f, 1.4f, 0.8f}, 1) == vec3{0, 1, 1});
    PLY_TEST_CHECK(round_nearest(vec3{0.3f, -1.4f, -0.8f}, 1) == vec3{0, -1, -1});
    PLY_TEST_CHECK(round_nearest(vec3{-0.3f, 1.4f, 0.8f}, 0.5f) ==
                   vec3{-0.5f, 1.5f, 1});
}

PLY_TEST_CASE("vec3 roundUp") {
    PLY_TEST_CHECK(round_up(vec3{0.3f, 1.4f, 0.8f}, 2) == vec3{2, 2, 2});
    PLY_TEST_CHECK(round_up(vec3{-0.3f, 1.4f, 0.8f}, 1) == vec3{0, 2, 1});
    PLY_TEST_CHECK(round_up(vec3{0.3f, -1.4f, -0.8f}, 1) == vec3{1, -1, 0});
    PLY_TEST_CHECK(round_up(vec3{-0.3f, 1.4f, 0.8f}, 0.5f) == vec3{0, 1.5f, 1});
}

PLY_TEST_CASE("vec3 roundDown") {
    PLY_TEST_CHECK(round_down(vec3{0.3f, 1.4f, 0.8f}, 2) == vec3{0, 0, 0});
    PLY_TEST_CHECK(round_down(vec3{-0.3f, 1.4f, 0.8f}, 1) == vec3{-1, 1, 0});
    PLY_TEST_CHECK(round_down(vec3{0.3f, -1.4f, -0.8f}, 1) == vec3{0, -2, -1});
    PLY_TEST_CHECK(round_down(vec3{-0.3f, 1.4f, 0.8f}, 0.5f) == vec3{-0.5f, 1, 0.5f});
}

PLY_TEST_CASE("vec3 isRounded") {
    PLY_TEST_CHECK(is_rounded(vec3{0, 1, 2}, 1));
    PLY_TEST_CHECK(!is_rounded(vec3{0.5f, 0, 2}, 1));
    PLY_TEST_CHECK(!is_rounded(vec3{1, -0.5f, 0}, 1));
    PLY_TEST_CHECK(!is_rounded(vec3{0, 1, 0.5f}, 1));
    PLY_TEST_CHECK(is_rounded(vec3{0, 1, 2}, 0.5f));
    PLY_TEST_CHECK(is_rounded(vec3{0.5f, -0.f, -1.5f}, 0.5f));
    PLY_TEST_CHECK(is_rounded(vec3{1, -0.5f, 0.f}, 0.5f));
    PLY_TEST_CHECK(!is_rounded(vec3{-0.5f, 0.3f, 0.f}, 0.5f));
    PLY_TEST_CHECK(!is_rounded(vec3{0, 1, 2}, 2));
    PLY_TEST_CHECK(is_rounded(vec3{4, 8, -2}, 2));
}

} // namespace ply

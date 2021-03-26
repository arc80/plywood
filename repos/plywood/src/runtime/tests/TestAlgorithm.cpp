/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-test/TestSuite.h>
#include <ply-runtime/algorithm/Map.h>
#include <ply-runtime/algorithm/Range.h>
#include <ply-runtime/algorithm/Find.h>
#include <ply-runtime/algorithm/Sum.h>

namespace ply {
namespace tests {

#define PLY_TEST_CASE_PREFIX Algorithm_

PLY_TEST_CASE("map() from Array") {
    Array<u32> arr = {1, 2, 3};
    Array<u32> result = map(arr, [](u32 x) { return x + 10; });
    PLY_TEST_CHECK(result == ArrayView<const u32>{11, 12, 13});
}

PLY_TEST_CASE("map() from ArrayView") {
    Array<u32> result = map(ArrayView<const u32>{1, 2, 3}, [](u32 x) { return x + 10; });
    PLY_TEST_CHECK(result == ArrayView<const u32>{11, 12, 13});
}

PLY_TEST_CASE("map() from FixedArray") {
    FixedArray<u32, 3> arr = {1, 2, 3};
    Array<u32> result = map(arr, [](u32 x) { return x + 10; });
    PLY_TEST_CHECK(result == ArrayView<const u32>{11, 12, 13});
}

PLY_TEST_CASE("map() from C-style array") {
    u32 arr[] = {1, 2, 3};
    Array<u32> result = map(arr, [](u32 x) { return x + 10; });
    PLY_TEST_CHECK(result == ArrayView<const u32>{11, 12, 13});
}

PLY_TEST_CASE("map() from range()") {
    Array<u32> result = map(range(3), [](u32 x) { return x + 10; });
    PLY_TEST_CHECK(result == ArrayView<const u32>{10, 11, 12});
}

PLY_TEST_CASE("map() from FileSystem::listDir()") {
    auto pair = NativePath::split(__FILE__);
    Array<String> result =
        map(FileSystem::native()->listDir(pair.first), [](const auto& e) { return e.name; });
    PLY_TEST_CHECK(find(result, pair.second) >= 0);
}

PLY_TEST_CASE("sum() u32s") {
    u32 s = sum(ArrayView<const u32>{1, 2, 3});
    PLY_TEST_CHECK(s == 6);
}

PLY_TEST_CASE("sum() C-style array of doubles") {
    double arr[] = {5, 6, 7};
    double s = sum(arr);
    PLY_TEST_CHECK(s == 18);
}

PLY_TEST_CASE("sum() over range") {
    u32 s = sum(range(1, 5));
    PLY_TEST_CHECK(s == 10);
}

PLY_TEST_CASE("reduce() multiply u32s") {
    u32 s = reduce(ArrayView<const u32>{2, 3, 4}, [](u32 a, u32 b) { return a * b; }, 1);
    PLY_TEST_CHECK(s == 24);
}

PLY_TEST_CASE("reduce() multiply floats") {
    float s = reduce(ArrayView<const float>{2, 3, 4}, [](auto a, auto b) { return a * b; }, 1);
    PLY_TEST_CHECK(s == 24);
}

PLY_TEST_CASE("reduce() multiply over range") {
    u32 s = reduce(range(1, 5), [](u32 a, u32 b) { return a * b; }, 1);
    PLY_TEST_CHECK(s == 24);
}


} // namespace tests
} // namespace ply

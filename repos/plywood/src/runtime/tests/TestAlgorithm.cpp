/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-test/TestSuite.h>
#include <ply-runtime/algorithm/Map.h>
#include <ply-runtime/algorithm/Range.h>
#include <ply-runtime/algorithm/Find.h>

namespace ply {
namespace tests {

#define PLY_TEST_CASE_PREFIX Algorithm_

PLY_TEST_CASE("map() from Array") {
    Array<u32> arr = {1, 2, 3};
    Array<u32> result = map(arr, [](u32 x) { return x + 10; });
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

} // namespace tests
} // namespace ply

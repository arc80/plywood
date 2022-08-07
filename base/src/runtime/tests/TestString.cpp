/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-runtime/string/String.h>
#include <ply-test/TestSuite.h>

namespace ply {
namespace tests {

#define PLY_TEST_CASE_PREFIX String_

PLY_TEST_CASE("String findByte") {
    String str = "abcdefgh";
    PLY_TEST_CHECK(str.findByte([](char x) { return x == 'c'; }) == 2);
    PLY_TEST_CHECK(str.findByte([](char x) { return x == 'z'; }) < 0);
    PLY_TEST_CHECK(str.findByte('c') == 2);
    PLY_TEST_CHECK(str.findByte('z') < 0);
}

PLY_TEST_CASE("String rfindByte") {
    String str = "abcdefgh";
    PLY_TEST_CHECK(str.rfindByte([](char x) { return x == 'c'; }) == 2);
    PLY_TEST_CHECK(str.rfindByte([](char x) { return x == 'z'; }) < 0);
    PLY_TEST_CHECK(str.rfindByte('c') == 2);
    PLY_TEST_CHECK(str.rfindByte('z') < 0);
}

} // namespace tests
} // namespace ply

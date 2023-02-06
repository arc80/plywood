/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-runtime.h>
#include <ply-test/TestSuite.h>

namespace ply {
namespace tests {

#define PLY_TEST_CASE_PREFIX String_

PLY_TEST_CASE("String self-assignment") {
    String str = "How now brown cow?";
    str = str.shortened_by(1);
    PLY_TEST_CHECK(str == "How now brown cow");
}

PLY_TEST_CASE("String findByte") {
    String str = "abcdefgh";
    PLY_TEST_CHECK(str.find_byte([](char x) { return x == 'c'; }) == 2);
    PLY_TEST_CHECK(str.find_byte([](char x) { return x == 'z'; }) < 0);
    PLY_TEST_CHECK(str.find_byte('c') == 2);
    PLY_TEST_CHECK(str.find_byte('z') < 0);
}

PLY_TEST_CASE("String rfindByte") {
    String str = "abcdefgh";
    PLY_TEST_CHECK(str.rfind_byte([](char x) { return x == 'c'; }) == 2);
    PLY_TEST_CHECK(str.rfind_byte([](char x) { return x == 'z'; }) < 0);
    PLY_TEST_CHECK(str.rfind_byte('c') == 2);
    PLY_TEST_CHECK(str.rfind_byte('z') < 0);
}

} // namespace tests
} // namespace ply

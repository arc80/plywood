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

#define PLY_TEST_CASE_PREFIX Encoding_

PLY_TEST_CASE("Decode truncated UTF-8") {
    // e3 80 82 is the valid UTF-8 encoding of U+3002
    // This is the truncated version of it
    // As such, it should be decoded as two 8-bit characters
    StringView badUTF8 = "\xe3\x80";
    ViewInStream in{badUTF8};
    Unicode decoder{UTF8};
    PLY_TEST_CHECK(decoder.decode_point(in) == 0xe3);
    PLY_TEST_CHECK(decoder.decode_point(in) == 0x80);
    PLY_TEST_CHECK(decoder.decode_point(in) < 0);
}

} // namespace tests
} // namespace ply

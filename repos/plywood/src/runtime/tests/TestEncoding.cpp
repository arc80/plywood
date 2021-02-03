/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-runtime/io/text/TextConverter.h>
#include <ply-test/TestSuite.h>

namespace ply {
namespace tests {

#define PLY_TEST_CASE_PREFIX Encoding_

String convertWithOutPipe(StringView buf, const TextEncoding* srcEnc, const TextEncoding* dstEnc) {
    MemOutStream mout;
    OutPipe_TextConverter conv{borrow(&mout), dstEnc, srcEnc};
    conv.write(buf);
    conv.flushMem();
    return mout.moveToString();
}

PLY_TEST_CASE("Decode truncated UTF-8") {
    // e3 80 82 is the valid UTF-8 encoding of U+3002
    // This is the truncated version of it
    // As such, it should be decoded as two 8-bit characters
    StringView badUTF8 = "\xe3\x80";
    String result = convertWithOutPipe(badUTF8, TextEncoding::get<UTF8>(), TextEncoding::get<UTF16_LE>());
    PLY_TEST_CHECK(result == StringView{"\xe3\x00\x80\x00", 4});
}

} // namespace tests
} // namespace ply

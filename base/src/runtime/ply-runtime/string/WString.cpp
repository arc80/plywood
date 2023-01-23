/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-runtime/Precomp.h>
#include <ply-runtime/string/WString.h>
#include <ply-runtime/string/TextEncoding.h>

namespace ply {

WString toWString(StringView str) {
    ViewInStream str_in{str};
    OutPipe_ConvertUnicode encoder{MemOutStream{}, UTF16_LE};
    encoder.write(str);
    encoder.out.raw_write<u16>(0); // Null terminator
    return WString::moveFromString(encoder.out.moveToString());
}

String fromWString(WStringView str) {
    InPipe_ConvertUnicode decoder{ViewInStream{str.raw_bytes()}, UTF16_LE};
    MemOutStream out;
    while (out.ensure_writable()) {
        MutStringView buf = out.view_writable();
        u32 num_bytes = decoder.read(buf);
        if (num_bytes == 0)
            break;
        out.cur_byte += num_bytes;
    }
    return out.moveToString();
}

} // namespace ply

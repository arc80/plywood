/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-runtime/Precomp.h>
#include <ply-runtime/string/WString.h>

namespace ply {

WString to_wstring(StringView str) {
    ViewInStream str_in{str};
    OutPipe_ConvertUnicode encoder{MemOutStream{}, UTF16_LE};
    encoder.write(str);
    encoder.child_stream.raw_write<u16>(0); // Null terminator
    return WString::move_from_string(encoder.child_stream.move_to_string());
}

String from_wstring(WStringView str) {
    InPipe_ConvertUnicode decoder{ViewInStream{str.raw_bytes()}, UTF16_LE};
    MemOutStream out;
    while (out.ensure_writable()) {
        MutStringView buf = out.view_writable();
        u32 num_bytes = decoder.read(buf);
        if (num_bytes == 0)
            break;
        out.cur_byte += num_bytes;
    }
    return out.move_to_string();
}

} // namespace ply

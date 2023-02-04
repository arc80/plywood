/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-runtime/Precomp.h>
#include <ply-runtime/io/text/LiquidTags.h>

namespace ply {

PLY_NO_INLINE void extractLiquidTags(OutStream& out, ViewInStream& vin,
                                     Func<void(StringView, StringView)> tagHandler) {
    // advanceByte consumes the current byte from the InStream and returns true if there is
    // another byte available. Note that the return value is the same as vin.isValid(), so you
    // can ignore it and just test vin.isValid() later.
    auto advanceByte = [&]() -> bool {
        PLY_ASSERT(vin.num_bytes_readable() > 0);
        vin.cur_byte++;
        return vin.ensure_readable();
    };

    if (vin.ensure_readable() == 0)
        return; // Empty file

    for (;;) {
        PLY_ASSERT(
            vin.num_bytes_readable() >
            0); // At start of this loop, there is always a byte available to read
        u8 unit = *vin.cur_byte;
        if (unit == '<') {
            const char* startByte = vin.cur_byte;
            if (!advanceByte()) {
                out << unit;
                return;
            }
            if (*vin.cur_byte == '%') {
                // Start of tag
                if (!advanceByte()) {
                    // FIXME: Raise error: EOF between tags
                    return;
                }
                MemOutStream mout;
                // read everything up to %>
                for (;;) {
                    PLY_ASSERT(vin.num_bytes_readable() >
                        0); // At start of this loop, there is always a byte available to read
                    unit = *vin.cur_byte;
                    if (unit == '%') {
                        if (!advanceByte()) {
                            // FIXME: Raise error: EOF between tags
                            return;
                        }
                        if (*vin.cur_byte == '>') {
                            advanceByte();
                            // End of tag
                            break;
                        } else {
                            mout << unit;
                            mout << *vin.cur_byte;
                        }
                    } else {
                        mout << unit;
                        if (!advanceByte()) {
                            // FIXME: Raise error: EOF between tags
                            break;
                        }
                    }
                }

                tagHandler(StringView::fromRange(startByte, vin.cur_byte), mout.moveToString());

                if (vin.at_eof()) {
                    // EOF encountered immediately after closing tag
                    return;
                }
            } else {
                out << unit;
                out << *vin.cur_byte;
                if (!advanceByte())
                    return; // EOF
            }
        } else {
            out << unit;
            if (!advanceByte())
                return; // EOF
        }
    }
}

} // namespace ply

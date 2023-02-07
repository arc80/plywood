/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-runtime/Precomp.h>
#include <ply-runtime/io/text/LiquidTags.h>

namespace ply {

void extract_liquid_tags(OutStream& out, ViewInStream& vin,
                         Func<void(StringView, StringView)> tag_handler) {
    // advance_byte consumes the current byte from the InStream and returns true if
    // there is another byte available. Note that the return value is the same as
    // vin.is_valid(), so you can ignore it and just test vin.is_valid() later.
    auto advance_byte = [&]() -> bool {
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
            const char* start_byte = vin.cur_byte;
            if (!advance_byte()) {
                out << unit;
                return;
            }
            if (*vin.cur_byte == '%') {
                // Start of tag
                if (!advance_byte()) {
                    // FIXME: Raise error: EOF between tags
                    return;
                }
                MemOutStream mout;
                // read everything up to %>
                for (;;) {
                    PLY_ASSERT(vin.num_bytes_readable() >
                               0); // At start of this loop, there is always a byte
                                   // available to read
                    unit = *vin.cur_byte;
                    if (unit == '%') {
                        if (!advance_byte()) {
                            // FIXME: Raise error: EOF between tags
                            return;
                        }
                        if (*vin.cur_byte == '>') {
                            advance_byte();
                            // End of tag
                            break;
                        } else {
                            mout << unit;
                            mout << *vin.cur_byte;
                        }
                    } else {
                        mout << unit;
                        if (!advance_byte()) {
                            // FIXME: Raise error: EOF between tags
                            break;
                        }
                    }
                }

                tag_handler(StringView::from_range(start_byte, vin.cur_byte),
                            mout.move_to_string());

                if (vin.at_eof()) {
                    // EOF encountered immediately after closing tag
                    return;
                }
            } else {
                out << unit;
                out << *vin.cur_byte;
                if (!advance_byte())
                    return; // EOF
            }
        } else {
            out << unit;
            if (!advance_byte())
                return; // EOF
        }
    }
}

} // namespace ply

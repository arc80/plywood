/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-runtime/Precomp.h>
#include <ply-runtime/io/text/LiquidTags.h>
#include <ply-runtime/io/OutStream.h>

namespace ply {

PLY_NO_INLINE void extractLiquidTags(OutStream* outs, ViewInStream* vins,
                                     Functor<void(StringView, StringView)> tagHandler) {
    // advanceByte consumes the current byte from the InStream and returns true if there is
    // another byte available. Note that the return value is the same as vins->isValid(), so you
    // can ignore it and just test vins->isValid() later.
    auto advanceByte = [&]() -> bool {
        PLY_ASSERT(vins->numBytesAvailable() > 0);
        vins->curByte++;
        return vins->tryMakeBytesAvailable() > 0;
    };

    if (vins->tryMakeBytesAvailable() == 0)
        return; // Empty file

    for (;;) {
        PLY_ASSERT(vins->numBytesAvailable() >
                   0); // At start of this loop, there is always a byte available to read
        u8 unit = *vins->curByte;
        if (unit == '<') {
            const char* startByte = vins->curByte;
            if (!advanceByte()) {
                outs->writeByte(unit);
                return;
            }
            if (*vins->curByte == '%') {
                // Start of tag
                if (!advanceByte()) {
                    // FIXME: Raise error: EOF between tags
                    return;
                }
                MemOutStream mout;
                // read everything up to %>
                for (;;) {
                    PLY_ASSERT(
                        vins->numBytesAvailable() >
                        0); // At start of this loop, there is always a byte available to read
                    unit = *vins->curByte;
                    if (unit == '%') {
                        if (!advanceByte()) {
                            // FIXME: Raise error: EOF between tags
                            return;
                        }
                        if (*vins->curByte == '>') {
                            advanceByte();
                            // End of tag
                            break;
                        } else {
                            mout.writeByte(unit);
                            mout.writeByte(*vins->curByte);
                        }
                    } else {
                        mout.writeByte(unit);
                        if (!advanceByte()) {
                            // FIXME: Raise error: EOF between tags
                            break;
                        }
                    }
                }

                tagHandler(StringView::fromRange(startByte, vins->curByte), mout.moveToString());

                if (vins->atEOF()) {
                    // EOF encountered immediately after closing tag
                    return;
                }
            } else {
                outs->writeByte(unit);
                outs->writeByte(*vins->curByte);
                if (!advanceByte())
                    return; // EOF
            }
        } else {
            outs->writeByte(unit);
            if (!advanceByte())
                return; // EOF
        }
    }
}

} // namespace ply

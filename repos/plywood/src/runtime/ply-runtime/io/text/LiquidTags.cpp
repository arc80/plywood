/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-runtime/Precomp.h>
#include <ply-runtime/io/text/LiquidTags.h>
#include <ply-runtime/io/OutStream.h>

namespace ply {

PLY_NO_INLINE void extractLiquidTags(StringWriter* sw, StringViewReader* svr,
                                     Functor<void(StringView, StringView)> tagHandler) {
    // advanceByte consumes the current byte from the InStream and returns true if there is
    // another byte available. Note that the return value is the same as svr->isValid(), so you
    // can ignore it and just test svr->isValid() later.
    auto advanceByte = [&]() -> bool {
        PLY_ASSERT(svr->numBytesAvailable() > 0);
        svr->curByte++;
        return svr->tryMakeBytesAvailable() > 0;
    };

    if (svr->tryMakeBytesAvailable() == 0)
        return; // Empty file

    for (;;) {
        PLY_ASSERT(svr->numBytesAvailable() >
                   0); // At start of this loop, there is always a byte available to read
        u8 unit = *svr->curByte;
        if (unit == '<') {
            const char* startByte = svr->curByte;
            if (!advanceByte()) {
                sw->writeByte(unit);
                return;
            }
            if (*svr->curByte == '%') {
                // Start of tag
                if (!advanceByte()) {
                    // FIXME: Raise error: EOF between tags
                    return;
                }
                MemOutStream mout;
                // read everything up to %>
                for (;;) {
                    PLY_ASSERT(
                        svr->numBytesAvailable() >
                        0); // At start of this loop, there is always a byte available to read
                    unit = *svr->curByte;
                    if (unit == '%') {
                        if (!advanceByte()) {
                            // FIXME: Raise error: EOF between tags
                            return;
                        }
                        if (*svr->curByte == '>') {
                            advanceByte();
                            // End of tag
                            break;
                        } else {
                            mout.writeByte(unit);
                            mout.writeByte(*svr->curByte);
                        }
                    } else {
                        mout.writeByte(unit);
                        if (!advanceByte()) {
                            // FIXME: Raise error: EOF between tags
                            break;
                        }
                    }
                }

                tagHandler.call(
                    StringView::fromRange(startByte, svr->curByte),
                    mout.moveToString());

                if (svr->atEOF()) {
                    // EOF encountered immediately after closing tag
                    return;
                }
            } else {
                sw->writeByte(unit);
                sw->writeByte(*svr->curByte);
                if (!advanceByte())
                    return; // EOF
            }
        } else {
            sw->writeByte(unit);
            if (!advanceByte())
                return; // EOF
        }
    }
}

} // namespace ply

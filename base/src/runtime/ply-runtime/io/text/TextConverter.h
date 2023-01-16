/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/io/Pipe.h>
#include <ply-runtime/io/InStream.h>
#include <ply-runtime/io/OutStream.h>
#include <ply-runtime/string/TextEncoding.h>
#include <ply-runtime/string/String.h>
#include <ply-runtime/string/WString.h>

namespace ply {

//-----------------------------------------------------------------------
// TextConverter
//-----------------------------------------------------------------------
struct TextConverter {
    struct SmallBuffer {
        char bytes[4] = {0};
        u8 numBytes = 0;

        PLY_INLINE MutStringView view() {
            return {(char*) this->bytes, this->numBytes};
        }
        PLY_INLINE void popFront(u32 numBytesToPop) {
            PLY_ASSERT(numBytesToPop <= this->numBytes);
            memmove(this->bytes, this->bytes + numBytesToPop,
                    this->numBytes - numBytesToPop);
            this->numBytes -= numBytesToPop;
        }
    };

    const TextEncoding* dstEncoding = nullptr;
    const TextEncoding* srcEncoding = nullptr;
    SmallBuffer srcSmallBuf;
    SmallBuffer dstSmallBuf;

    PLY_DLL_ENTRY TextConverter(const TextEncoding* dstEncoding,
                                const TextEncoding* srcEncoding);
    template <typename DstEnc, typename SrcEnc>
    PLY_INLINE static TextConverter create() {
        return {TextEncoding::get<DstEnc>(), TextEncoding::get<SrcEnc>()};
    }

    // Convert using memory buffers
    // Returns true if any work done (reading or writing)
    // When flush is true, consumes as much srcBuf as possible, even if it contains a
    // partially encoded point.
    PLY_DLL_ENTRY bool convert(MutStringView* dstBuf, StringView* srcBuf, bool flush);

    // Read/write to a Stream
    // Returns true if any work done (reading or writing)
    // When flush is true, consumes as much srcBuf as possible, even if it contains a
    // partially encoded point.
    PLY_DLL_ENTRY bool writeTo(OutStream& out, StringView* srcBuf, bool flush);
    PLY_DLL_ENTRY u32 readFrom(InStream& in, MutStringView* dstBuf);

    // Convert a string
    PLY_DLL_ENTRY static String convertInternal(const TextEncoding* dstEncoding,
                                                const TextEncoding* srcEncoding,
                                                StringView srcText);
    template <typename DstEnc, typename SrcEnc>
    PLY_INLINE static String convert(StringView srcText) {
        return convertInternal(TextEncoding::get<DstEnc>(), TextEncoding::get<SrcEnc>(),
                               srcText);
    }
};

//-----------------------------------------------------------------------
// InPipe_TextConverter
//-----------------------------------------------------------------------
struct InPipe_TextConverter : InPipe {
    InStream in;
    TextConverter converter;

    InPipe_TextConverter(InStream&& in, const TextEncoding* dstEncoding,
                         const TextEncoding* srcEncoding)
        : in{std::move(in)}, converter{dstEncoding, srcEncoding} {
    }
    virtual u32 read(MutStringView buf) override;
};

//-----------------------------------------------------------------------
// OutPipe_TextConverter
//-----------------------------------------------------------------------
struct OutPipe_TextConverter : OutPipe {
    OutStream out;
    TextConverter converter;

    OutPipe_TextConverter(OutStream&& out, const TextEncoding* dstEncoding,
                          const TextEncoding* srcEncoding)
        : out{std::move(out)}, converter{dstEncoding, srcEncoding} {
    }
    virtual bool write(StringView buf) override;
    virtual void flush(bool hard);
};

} // namespace ply

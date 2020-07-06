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
        u8 bytes[4] = {0};
        u8 numBytes = 0;

        PLY_INLINE BufferView view() {
            return {this->bytes, this->numBytes};
        }
        PLY_INLINE void popFront(u32 numBytesToPop) {
            PLY_ASSERT(numBytesToPop <= this->numBytes);
            memmove(this->bytes, this->bytes + numBytesToPop, this->numBytes - numBytesToPop);
            this->numBytes -= numBytesToPop;
        }
    };

    const TextEncoding* dstEncoding = nullptr;
    const TextEncoding* srcEncoding = nullptr;
    SmallBuffer srcSmallBuf;
    SmallBuffer dstSmallBuf;

    PLY_DLL_ENTRY TextConverter(const TextEncoding* dstEncoding, const TextEncoding* srcEncoding);
    template <typename DstEnc, typename SrcEnc>
    PLY_INLINE static TextConverter create() {
        return {TextEncoding::get<DstEnc>(), TextEncoding::get<SrcEnc>()};
    }

    // Convert using memory buffers
    // Returns true if any work done (reading or writing)
    PLY_DLL_ENTRY bool convert(BufferView* dstBuf, ConstBufferView* srcBuf, bool flush);

    // Read/write to a Stream
    // Returns true if any work done (reading or writing)
    PLY_DLL_ENTRY bool writeTo(OutStream* outs, ConstBufferView* srcBuf, bool flush);
    PLY_DLL_ENTRY u32 readFrom(InStream* ins, BufferView* dstBuf);

    // Convert a string
    PLY_DLL_ENTRY static Buffer convertInternal(const TextEncoding* dstEncoding,
                                                const TextEncoding* srcEncoding,
                                                ConstBufferView srcText);
    template <typename DstEnc, typename SrcEnc>
    PLY_INLINE static typename DstEnc::Container convert(typename SrcEnc::Container::View srcText) {
        return DstEnc::Container::moveFromBuffer(convertInternal(
            TextEncoding::get<DstEnc>(), TextEncoding::get<SrcEnc>(), srcText.bufferView()));
    }
};

//-----------------------------------------------------------------------
// InPipe_TextConverter
//-----------------------------------------------------------------------
struct InPipe_TextConverter : InPipe {
    static Funcs Funcs_;
    OptionallyOwned<InStream> ins;
    TextConverter converter;

    PLY_DLL_ENTRY InPipe_TextConverter(OptionallyOwned<InStream>&& ins,
                                       const TextEncoding* dstEncoding,
                                       const TextEncoding* srcEncoding);
};

//-----------------------------------------------------------------------
// OutPipe_TextConverter
//-----------------------------------------------------------------------
struct OutPipe_TextConverter : OutPipe {
    static Funcs Funcs_;
    OptionallyOwned<OutStream> outs;
    TextConverter converter;

    PLY_DLL_ENTRY OutPipe_TextConverter(OptionallyOwned<OutStream>&& outs,
                                        const TextEncoding* dstEncoding,
                                        const TextEncoding* srcEncoding);
};

} // namespace ply

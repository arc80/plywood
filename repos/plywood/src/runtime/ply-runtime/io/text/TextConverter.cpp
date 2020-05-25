/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-runtime/Precomp.h>
#include <ply-runtime/io/text/TextConverter.h>
#include <ply-runtime/io/OutStream.h>

namespace ply {

//-----------------------------------------------------------------------
// TextConverter
//-----------------------------------------------------------------------
PLY_NO_INLINE TextConverter::TextConverter(const TextEncoding* dstEncoding,
                                           const TextEncoding* srcEncoding)
    : dstEncoding{dstEncoding}, srcEncoding{srcEncoding} {
}

PLY_NO_INLINE bool TextConverter::convert(BufferView* dstBuf, ConstBufferView* srcBuf, bool flush) {
    bool didWork = false;
    while (dstBuf->numBytes > 0) {
        // First, try to copy any bytes that have been buffered into smallBuf.
        u32 numBytesToCopy = this->smallBuf.numAvailableBytes();
        if (numBytesToCopy > 0) {
            numBytesToCopy = min(numBytesToCopy, dstBuf->numBytes);
            if (numBytesToCopy > 0) {
                memcpy(dstBuf->bytes, this->smallBuf.curByte(), numBytesToCopy);
                this->smallBuf.curPos += numBytesToCopy;
                dstBuf->offsetHead(numBytesToCopy);
                didWork = true;
            }
            if (dstBuf->numBytes == 0)
                break; // Output buf has been filled.
        }

        // Don't attempt to decode unless there are at least 4 bytes (unless we're flushing the
        // src).
        if (srcBuf->numBytes < 4) {
            if (srcBuf->numBytes == 0 || !flush)
                break;
        }

        // Decode one point from the input.
        DecodeResult decoded = this->srcEncoding->decodePoint(*srcBuf);
        srcBuf->offsetHead(decoded.numBytes);
        didWork = true;

        if (dstBuf->numBytes >= 4) {
            // There's enough room in the output buffer for *any* encoded point.
            // Encode it directly to the output buffer.
            u32 numBytesEncoded = this->dstEncoding->encodePoint(*dstBuf, decoded.point);
            dstBuf->offsetHead(numBytesEncoded);
        } else {
            // There may not be enough room in the output buffer to encode the entire
            // point. Write it temporarily to smallBuf instead.
            this->smallBuf.curPos = 0;
            this->smallBuf.endPos = (u8) this->dstEncoding->encodePoint(
                {this->smallBuf.buf, PLY_STATIC_ARRAY_SIZE(this->smallBuf.buf)}, decoded.point);
        }
    }
    return didWork;
}

PLY_NO_INLINE bool TextConverter::writeTo(OutStream* outs, ConstBufferView* srcBuf, bool flush) {
    bool anyWorkDone = false;
    for (;;) {
        if (!outs->tryMakeBytesAvailable())
            break;

        // Write as much output as we can
        BufferView dstBuf = outs->viewAvailable();
        bool didWork = this->convert(&dstBuf, srcBuf, flush);
        outs->curByte = dstBuf.bytes;
        if (!didWork)
            break;
        anyWorkDone = true;
    }
    return anyWorkDone;
}

PLY_NO_INLINE u32 TextConverter::readFrom(InStream* ins, BufferView* dstBuf) {
    u32 totalBytesWritten = 0;
    for (;;) {
        // FIXME: Make this algorithm tighter!
        // Advancing the input is a potentially blocking operation, so only do it if we
        // absolutely have to:
        ins->tryMakeBytesAvailable(4); // will return less than 4 on EOF/error *ONLY*

        // Filter as much input as we can:
        u8* dstBefore = dstBuf->bytes;
        ConstBufferView srcBuf = ins->viewAvailable();
        bool flush = ins->atEOF();
        this->convert(dstBuf, &srcBuf, flush);
        ins->curByte = (u8*) srcBuf.bytes;
        s32 numBytesWritten = safeDemote<s32>(dstBuf->bytes - dstBefore);
        totalBytesWritten += numBytesWritten;

        // If anything was written, stop.
        if (numBytesWritten > 0)
            break;
        // If input was exhausted, stop.
        if (flush) {
            PLY_ASSERT(ins->numBytesAvailable() == 0);
            break;
        }
    }
    return totalBytesWritten;
}

PLY_NO_INLINE Buffer TextConverter::convertInternal(const TextEncoding* dstEncoding,
                                                    const TextEncoding* srcEncoding,
                                                    ConstBufferView srcText) {
    MemOutStream outs;
    TextConverter converter{dstEncoding, srcEncoding};
    converter.writeTo(&outs, &srcText, true);
    PLY_ASSERT(dstEncoding->unitSize > 0);
    return outs.moveToBuffer();
}

//-----------------------------------------------------------------------
// InPipe_TextConverter
//-----------------------------------------------------------------------
PLY_NO_INLINE void InPipe_TextConverter_destroy(InPipe* inPipe_) {
    InPipe_TextConverter* inPipe = static_cast<InPipe_TextConverter*>(inPipe_);
    destruct(inPipe->ins);
}

PLY_NO_INLINE u32 InPipe_TextConverter_readSome(InPipe* inPipe_, BufferView dstBuf) {
    InPipe_TextConverter* inPipe = static_cast<InPipe_TextConverter*>(inPipe_);
    return inPipe->converter.readFrom(inPipe->ins, &dstBuf);
}

InPipe::Funcs InPipe_TextConverter::Funcs_ = {
    InPipe_TextConverter_destroy,
    InPipe_TextConverter_readSome,
    InPipe::getFileSize_Unsupported,
};

PLY_NO_INLINE InPipe_TextConverter::InPipe_TextConverter(OptionallyOwned<InStream>&& ins,
                                                         const TextEncoding* dstEncoding,
                                                         const TextEncoding* srcEncoding)
    : InPipe{&Funcs_}, ins{std::move(ins)}, converter{dstEncoding, srcEncoding} {
}

//-----------------------------------------------------------------------
// OutPipe_TextConverter
//-----------------------------------------------------------------------
PLY_NO_INLINE void OutPipe_TextConverter_destroy(OutPipe* outPipe_) {
    OutPipe_TextConverter* outPipe = static_cast<OutPipe_TextConverter*>(outPipe_);
    destruct(outPipe->outs);
}

PLY_NO_INLINE bool OutPipe_TextConverter_write(OutPipe* outPipe_, ConstBufferView srcBuf) {
    OutPipe_TextConverter* outPipe = static_cast<OutPipe_TextConverter*>(outPipe_);
    outPipe->converter.writeTo(outPipe->outs, &srcBuf, false);
    return !outPipe->outs->atEOF();
}

PLY_NO_INLINE bool OutPipe_TextConverter_flush(OutPipe* outPipe_, bool toDevice) {
    OutPipe_TextConverter* outPipe = static_cast<OutPipe_TextConverter*>(outPipe_);
    return outPipe->outs->flush(toDevice);
}

OutPipe::Funcs OutPipe_TextConverter::Funcs_ = {
    OutPipe_TextConverter_destroy,
    OutPipe_TextConverter_write,
    OutPipe_TextConverter_flush,
    OutPipe::seek_Empty,
};

PLY_NO_INLINE OutPipe_TextConverter::OutPipe_TextConverter(OptionallyOwned<OutStream>&& outs,
                                                           const TextEncoding* dstEncoding,
                                                           const TextEncoding* srcEncoding)
    : OutPipe{&Funcs_}, outs{std::move(outs)}, converter{dstEncoding, srcEncoding} {
}

} // namespace ply

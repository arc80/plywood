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

PLY_NO_INLINE bool TextConverter::convert(MutableStringView* dstBuf, StringView* srcBuf,
                                          bool flush) {
    bool wroteAnything = false;

    auto flushDstSmallBuf = [&] {
        u32 numBytesToCopy = min((u32) this->dstSmallBuf.numBytes, dstBuf->numBytes);
        if (numBytesToCopy > 0) {
            memcpy(dstBuf->bytes, this->dstSmallBuf.bytes, numBytesToCopy);
            this->dstSmallBuf.popFront(numBytesToCopy);
            dstBuf->offsetHead(numBytesToCopy);
            wroteAnything = true;
        }
        return numBytesToCopy;
    };

    if (dstBuf->numBytes == 0)
        return wroteAnything; // dstBuf is full

    // First, try to copy any bytes that have been buffered into dstSmallBuf.
    if (this->dstSmallBuf.numBytes > 0) {
        flushDstSmallBuf();
        if (dstBuf->numBytes == 0)
            return wroteAnything; // dstBuf is full
    }

    while (this->srcSmallBuf.numBytes > 0) {
        PLY_ASSERT(this->dstSmallBuf.numBytes == 0);

        // Append more input to srcSmallBuf.
        // We might put it back after.
        u32 numCopiedFromSrc =
            min((u32) PLY_STATIC_ARRAY_SIZE(this->srcSmallBuf.bytes) - this->srcSmallBuf.numBytes,
                srcBuf->numBytes);
        memcpy(this->srcSmallBuf.bytes + this->srcSmallBuf.numBytes, srcBuf->bytes,
               numCopiedFromSrc);

        // Try to decode from srcSmallBuf.
        this->srcSmallBuf.numBytes += numCopiedFromSrc;
        DecodeResult decoded = this->srcEncoding->decodePoint(this->srcSmallBuf.view());
        this->srcSmallBuf.numBytes -= numCopiedFromSrc;

        if (decoded.status == DecodeResult::Status::Truncated) {
            // Not enough input units for a complete code point
            PLY_ASSERT(this->srcSmallBuf.numBytes < 4); // Sanity check
            PLY_ASSERT((decoded.point >= 0) == (decoded.numBytes > 0));
            // If flush is true and there's an (invalid) point to consume, we'll consume it below.
            // Otherwise, don't consume anything and return from here:
            if (!(flush && decoded.point >= 0))
                return wroteAnything;
        }
        PLY_ASSERT((decoded.point >= 0) && (decoded.numBytes > 0));

        // Encode this point to dstSmallBuf, then flush dstSmallBuf
        this->dstSmallBuf.numBytes = this->dstEncoding->encodePoint(
            {this->dstSmallBuf.bytes, PLY_STATIC_ARRAY_SIZE(this->dstSmallBuf.bytes)},
            decoded.point);
        PLY_ASSERT(this->dstSmallBuf.numBytes > 0);
        flushDstSmallBuf();

        // Advance input
        if (decoded.numBytes < this->srcSmallBuf.numBytes) {
            this->srcSmallBuf.popFront(decoded.numBytes);
            // Still reading from srcSmallBuf
        } else {
            // srcSmallBuf has been emptied
            srcBuf->offsetHead(decoded.numBytes - this->srcSmallBuf.numBytes);
            this->srcSmallBuf.numBytes = 0;
        }

        if (dstBuf->numBytes == 0)
            return wroteAnything; // dstBuf is full
    }

    // At this point, both dstSmallBuf and srcSmallBuf should be empty, which means that we can now
    // operate directly on srcBuf and dstBuf.
    PLY_ASSERT(dstBuf->numBytes > 0);
    PLY_ASSERT(this->dstSmallBuf.numBytes == 0);
    PLY_ASSERT(this->srcSmallBuf.numBytes == 0);

    while (srcBuf->numBytes > 0) {
        // Decode one point from the input.
        DecodeResult decoded = this->srcEncoding->decodePoint(*srcBuf);
        if (decoded.status == DecodeResult::Status::Truncated) {
            if (!flush) {
                // Not enough input units for a complete code point. Copy input to srcSmallBuf.
                PLY_ASSERT(srcBuf->numBytes < 4); // Sanity check
                memcpy(this->srcSmallBuf.bytes, srcBuf->bytes, srcBuf->numBytes);
                this->srcSmallBuf.numBytes = srcBuf->numBytes;
                srcBuf->offsetHead(srcBuf->numBytes);
                return wroteAnything;
            }
            PLY_ASSERT((decoded.point >= 0) == (decoded.numBytes > 0));
            if (decoded.point < 0)
                return wroteAnything;
        }
        PLY_ASSERT(decoded.point >= 0);

        // Consume input bytes.
        srcBuf->offsetHead(decoded.numBytes);

        if (dstBuf->numBytes >= 4) {
            // Encode directly to the output buffer.
            u32 numBytesEncoded = this->dstEncoding->encodePoint(*dstBuf, decoded.point);
            PLY_ASSERT(numBytesEncoded > 0);
            dstBuf->offsetHead(numBytesEncoded);
            wroteAnything = true;
        } else {
            // Encode to dstSmallBuf.
            this->dstSmallBuf.numBytes = this->dstEncoding->encodePoint(
                {this->dstSmallBuf.bytes, PLY_STATIC_ARRAY_SIZE(this->dstSmallBuf.bytes)},
                decoded.point);
            PLY_ASSERT(this->dstSmallBuf.numBytes > 0);

            // Flush dstSmallBuf.
            flushDstSmallBuf();
            if (dstBuf->numBytes == 0)
                return wroteAnything; // dstBuf has been filled.
        }
    }

    // No more input.
    return wroteAnything;
}

PLY_NO_INLINE bool TextConverter::writeTo(OutStream* outs, StringView* srcBuf, bool flush) {
    bool anyWorkDone = false;
    for (;;) {
        if (!outs->tryMakeBytesAvailable())
            break;

        // Write as much output as we can
        MutableStringView dstBuf = outs->viewAvailable();
        bool didWork = this->convert(&dstBuf, srcBuf, flush);
        outs->curByte = dstBuf.bytes;
        if (!didWork)
            break;
        anyWorkDone = true;
    }
    return anyWorkDone;
}

PLY_NO_INLINE u32 TextConverter::readFrom(InStream* ins, MutableStringView* dstBuf) {
    u32 totalBytesWritten = 0;
    for (;;) {
        // FIXME: Make this algorithm tighter!
        // Advancing the input is a potentially blocking operation, so only do it if we
        // absolutely have to:
        ins->tryMakeBytesAvailable(4); // will return less than 4 on EOF/error *ONLY*

        // Filter as much input as we can:
        char* dstBefore = dstBuf->bytes;
        StringView srcBuf = ins->viewAvailable();
        bool flush = ins->atEOF();
        this->convert(dstBuf, &srcBuf, flush);
        ins->curByte = srcBuf.bytes;
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

PLY_NO_INLINE String TextConverter::convertInternal(const TextEncoding* dstEncoding,
                                                    const TextEncoding* srcEncoding,
                                                    StringView srcText) {
    MemOutStream outs;
    TextConverter converter{dstEncoding, srcEncoding};
    converter.writeTo(&outs, &srcText, true);
    PLY_ASSERT(dstEncoding->unitSize > 0);
    return outs.moveToString();
}

//-----------------------------------------------------------------------
// InPipe_TextConverter
//-----------------------------------------------------------------------
PLY_NO_INLINE void InPipe_TextConverter_destroy(InPipe* inPipe_) {
    InPipe_TextConverter* inPipe = static_cast<InPipe_TextConverter*>(inPipe_);
    destruct(inPipe->ins);
}

PLY_NO_INLINE u32 InPipe_TextConverter_readSome(InPipe* inPipe_, MutableStringView dstBuf) {
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

PLY_NO_INLINE bool OutPipe_TextConverter_write(OutPipe* outPipe_, StringView srcBuf) {
    OutPipe_TextConverter* outPipe = static_cast<OutPipe_TextConverter*>(outPipe_);
    outPipe->converter.writeTo(outPipe->outs, &srcBuf, false);
    return !outPipe->outs->atEOF();
}

PLY_NO_INLINE bool OutPipe_TextConverter_flush(OutPipe* outPipe_, bool toDevice) {
    OutPipe_TextConverter* outPipe = static_cast<OutPipe_TextConverter*>(outPipe_);
    StringView emptySrcBuf;
    outPipe->converter.writeTo(outPipe->outs, &emptySrcBuf, true);
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

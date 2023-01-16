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

PLY_NO_INLINE bool TextConverter::convert(MutStringView* dstBuf, StringView* srcBuf,
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

PLY_NO_INLINE bool TextConverter::writeTo(OutStream& out, StringView* srcBuf, bool flush) {
    bool anyWorkDone = false;
    for (;;) {
        if (!out.ensure_writable())
            break;

        // Write as much output as we can
        MutStringView dstBuf = out.view_writable();
        bool didWork = this->convert(&dstBuf, srcBuf, flush);
        out.cur_byte = dstBuf.bytes;
        if (!didWork)
            break;
        anyWorkDone = true;
    }
    return anyWorkDone;
}

PLY_NO_INLINE u32 TextConverter::readFrom(InStream& in, MutStringView* dstBuf) {
    u32 totalBytesWritten = 0;
    for (;;) {
        in.ensure_contiguous(4);

        // Filter as much input as we can:
        char* dstBefore = dstBuf->bytes;
        StringView srcBuf = in.view_readable();
        bool flush = in.at_eof();
        this->convert(dstBuf, &srcBuf, flush);
        in.cur_byte = srcBuf.bytes;
        s32 numBytesWritten = safeDemote<s32>(dstBuf->bytes - dstBefore);
        totalBytesWritten += numBytesWritten;

        // If anything was written, stop.
        if (numBytesWritten > 0)
            break;
        // If input was exhausted, stop.
        if (flush) {
            PLY_ASSERT(in.num_bytes_readable() == 0);
            break;
        }
    }
    return totalBytesWritten;
}

PLY_NO_INLINE String TextConverter::convertInternal(const TextEncoding* dstEncoding,
                                                    const TextEncoding* srcEncoding,
                                                    StringView srcText) {
    MemOutStream out;
    TextConverter converter{dstEncoding, srcEncoding};
    converter.writeTo(out, &srcText, true);
    PLY_ASSERT(dstEncoding->unitSize > 0);
    return out.moveToString();
}

//-----------------------------------------------------------------------
// InPipe_TextConverter
//-----------------------------------------------------------------------
u32 InPipe_TextConverter::read(MutStringView buf) {
    return this->converter.readFrom(this->in, &buf);
}

//-----------------------------------------------------------------------
// OutPipe_TextConverter
//-----------------------------------------------------------------------
bool OutPipe_TextConverter::write(StringView buf) {
    return this->converter.writeTo(this->out, &buf, false);
}

void OutPipe_TextConverter::flush(bool hard) {
    StringView emptySrcBuf;
    this->converter.writeTo(this->out, &emptySrcBuf, true);
    this->out.flush(hard);
}

} // namespace ply

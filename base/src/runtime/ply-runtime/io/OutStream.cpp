/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-runtime/Precomp.h>
#include <ply-runtime/io/OutStream.h>

namespace ply {

//------------------------------------------------------------------
// OutStream
//------------------------------------------------------------------
PLY_NO_INLINE void OutStream::initFirstBlock() {
    u32 blockSize = this->getBlockSize();
    new (&this->block) Reference<BlockList::Footer>{BlockList::createBlock(blockSize)};
    this->curByte = this->block->bytes;
    this->endByte = this->block->end();
}

PLY_NO_INLINE void makeNull(OutStream* outs) {
    outs->startByte = nullptr;
    outs->curByte = nullptr;
    outs->endByte = nullptr;
    outs->reserved = nullptr;
    outs->status = OutStream::Status{OutStream::Type::View, 0};
}

PLY_NO_INLINE OutStream::OutStream(OutStream&& other) : status{other.status} {
    this->curByte = other.curByte;
    this->endByte = other.endByte;
    this->startByte = other.startByte; // Assume all union members are same size
    this->reserved = other.reserved;   // Assume all union members are same size
    makeNull(&other);
}

PLY_NO_INLINE OutStream::OutStream(OptionallyOwned<OutPipe>&& outPipe, u32 blockSizeExp)
    : status{Type::Pipe, blockSizeExp} {
    PLY_ASSERT(outPipe);
    this->status.isPipeOwner = outPipe.isOwned() ? 1 : 0;
    this->outPipe = outPipe.release();
    this->initFirstBlock();
}

PLY_NO_INLINE void OutStream::destructInternal() {
    PLY_ASSERT(this->status.type != (u32) Type::View);
    this->flushMem();
    destruct(this->block);
    if (this->status.type == (u32) Type::Pipe) {
        if (this->status.isPipeOwner) {
            delete this->outPipe;
        }
    } else {
        PLY_ASSERT(this->status.type == (u32) Type::Mem);
        destruct(this->headBlock);
    }
}

PLY_NO_INLINE bool OutStream::flushInternal() {
    if (this->status.type == (u32) Type::View)
        return true;

    // this->block->numBytesUsed is the last flushed offset
    PLY_ASSERT(this->endByte == this->block->bytes + this->block->blockSize);
    u32 newWritePos = safeDemote<u32>(this->curByte - this->block->bytes);
    PLY_ASSERT(newWritePos >= this->block->numBytesUsed);
    PLY_ASSERT(newWritePos <= this->block->blockSize);
    if (newWritePos > this->block->numBytesUsed) {
        if (this->status.type == (u32) Type::Pipe && this->status.eof == 0) {
            if (!this->outPipe->write({this->block->bytes + this->block->numBytesUsed,
                                       newWritePos - this->block->numBytesUsed})) {
                this->status.eof = 1;
            }
        }
        this->block->numBytesUsed = newWritePos;
    }

    return this->status.eof == 0;
}

PLY_NO_INLINE bool OutStream::flush(bool toDevice) {
    this->flushInternal();
    bool rc = true;
    if (this->status.type == (u32) Type::Pipe) {
        rc = this->outPipe->flush(toDevice);
    }
    return rc;
}

PLY_NO_INLINE u64 OutStream::getSeekPos() const {
    if (this->status.type == (u32) Type::View) {
        return safeDemote<u64>(this->curByte - this->startByte);
    } else {
        return this->block->fileOffset + safeDemote<u64>(this->curByte - this->block->bytes);
    }
}

PLY_NO_INLINE u32 OutStream::tryMakeBytesAvailableInternal(s32 numBytes) {
    // If argument is positive, function is allowed to make fewer bytes available in case of
    // EOF/error, and the actual number is returned. If argument is negative, function must always
    // make (at least) the requested number of bytes available for write, even if EOF/error is
    // encountered. You must call atEOF() later to determine whether EOF/error was actually
    // encountered. It is currently illegal to pass a negative argument to a ViewOutStream, but
    // support is possible to add later if needed.
    PLY_ASSERT(numBytes != 0);

    if (this->status.type == (u32) Type::View) {
        // Currently illegal to pass a negative argument to a ViewOutStream, but support could be
        // added later if needed.
        PLY_ASSERT(numBytes > 0);
        u32 bytesToReturn = safeDemote<u32>(this->endByte - this->curByte);
        if (bytesToReturn < (u32) numBytes) {
            this->status.eof = 1;
        }
        return bytesToReturn;
    }

    if (this->status.eof && numBytes > 0) {
        return 0;
    }

    if (!this->flushInternal() && numBytes > 0) {
        this->endByte = this->curByte;
        return 0;
    }

    // Get a new block to write to
    BlockList::appendBlockWithRecycle(this->block, max(this->getBlockSize(), (u32) abs(numBytes)));
    this->curByte = this->block->bytes;
    this->endByte = this->block->bytes + this->block->blockSize;
    return this->block->blockSize;
}

PLY_NO_INLINE bool OutStream::writeSlowPath(StringView src) {
    if (this->status.eof)
        return false;

    while (src.numBytes > 0) {
        if (!this->tryMakeBytesAvailable())
            return false;
        u32 toCopy = min<u32>(this->numBytesAvailable(), src.numBytes);
        memcpy(this->curByte, src.bytes, toCopy);
        this->curByte += toCopy;
        src.offsetHead(toCopy);
    }

    return true;
}

//------------------------------------------------------------------
// MemOutStream
//------------------------------------------------------------------
PLY_NO_INLINE MemOutStream::MemOutStream(u32 blockSizeExp) : OutStream{Type::Mem, blockSizeExp} {
    this->initFirstBlock();
    new (&this->headBlock) Reference<BlockList::Footer>{this->block};
}

PLY_NO_INLINE String MemOutStream::moveToString() {
    PLY_ASSERT(this->status.type == (u32) Type::Mem);

    // "Flush" the current write position to the block's numBytesUsed.
    u32 newWritePos = safeDemote<u32>(this->curByte - this->block->bytes);
    PLY_ASSERT(newWritePos >= this->block->numBytesUsed);
    PLY_ASSERT(newWritePos <= this->block->blockSize);
    this->block->numBytesUsed = newWritePos;

    // Release block references and create the String. Releasing the references allows
    // BlockList::toString() to optimize the case where there's only one block.
    this->block = nullptr;
    char* bytes = this->headBlock->bytes;
    String result = BlockList::toString({std::move(this->headBlock), bytes});

    // Leave this stream in the state of a null ViewOutStream
    makeNull(this);

    return result;
}

} // namespace ply

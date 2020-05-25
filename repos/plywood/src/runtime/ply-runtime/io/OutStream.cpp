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
PLY_NO_INLINE void OutStream::initFirstChunk() {
    u32 chunkSize = this->getChunkSize();
    new (&this->chunk) Reference<ChunkListNode>{ChunkListNode::allocate(0, chunkSize)};
    this->curByte = this->chunk->bytes;
    this->endByte = this->chunk->end();
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

PLY_NO_INLINE OutStream::OutStream(OptionallyOwned<OutPipe>&& outPipe, u32 chunkSizeExp)
    : status{Type::Pipe, chunkSizeExp} {
    PLY_ASSERT(outPipe);
    this->status.isPipeOwner = outPipe.isOwned() ? 1 : 0;
    this->outPipe = outPipe.release();
    this->initFirstChunk();
}

PLY_NO_INLINE void OutStream::destructInternal() {
    PLY_ASSERT(this->status.type != (u32) Type::View);
    this->flushMem();
    destruct(this->chunk);
    if (this->status.type == (u32) Type::Pipe) {
        if (this->status.isPipeOwner) {
            delete this->outPipe;
        }
    } else {
        PLY_ASSERT(this->status.type == (u32) Type::Mem);
        destruct(this->headChunk);
    }
}

PLY_NO_INLINE bool OutStream::flushInternal() {
    if (this->status.type == (u32) Type::View)
        return true;

    // this->chunk->writePos is the last flushed offset
    PLY_ASSERT(this->endByte == this->chunk->bytes + this->chunk->numBytes);
    u32 newWritePos = safeDemote<u32>(this->curByte - this->chunk->bytes);
    PLY_ASSERT(newWritePos >= this->chunk->writePos);
    PLY_ASSERT(newWritePos <= this->chunk->numBytes);
    if (newWritePos > this->chunk->writePos) {
        if (this->status.type == (u32) Type::Pipe && this->status.eof == 0) {
            if (!this->outPipe->write({this->chunk->bytes + this->chunk->writePos,
                                       newWritePos - this->chunk->writePos})) {
                this->status.eof = 1;
            }
        }
        this->chunk->writePos = newWritePos;
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
        return this->chunk->fileOffset + safeDemote<u64>(this->curByte - this->chunk->bytes);
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

    // Get a new chunk to write to
    ChunkListNode::addChunkToTail(this->chunk, max(this->getChunkSize(), (u32) abs(numBytes)));
    this->curByte = this->chunk->bytes;
    this->endByte = this->chunk->bytes + this->chunk->numBytes;
    return this->chunk->numBytes;
}

PLY_NO_INLINE bool OutStream::writeSlowPath(ConstBufferView src) {
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
PLY_NO_INLINE MemOutStream::MemOutStream(u32 chunkSizeExp) : OutStream{Type::Mem, chunkSizeExp} {
    this->initFirstChunk();
    new (&this->headChunk) Reference<ChunkListNode>{this->chunk};
}

PLY_NO_INLINE Buffer MemOutStream::moveToBuffer() {
    PLY_ASSERT(this->status.type == (u32) Type::Mem);

    // Flush writePos
    u32 newWritePos = safeDemote<u32>(this->curByte - this->chunk->bytes);
    PLY_ASSERT(newWritePos >= this->chunk->writePos);
    PLY_ASSERT(newWritePos <= this->chunk->numBytes);
    this->chunk->writePos = newWritePos;

    // Release chunk references and create Buffer. Releasing the references allows
    // binaryFromChunks() to optimize in the case where there's only one chunk: Chunk memory gets
    // trimmed and returned immediately, avoiding a memcpy.
    this->chunk = nullptr;
    u8* bytes = this->headChunk->bytes;
    Buffer result = Buffer::fromChunks({std::move(this->headChunk), bytes});

    // Leave this stream in the state of a null ViewOutStream
    makeNull(this);

    return result;
}

} // namespace ply

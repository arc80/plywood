/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-runtime/Precomp.h>
#include <ply-runtime/io/InStream.h>
#include <ply-runtime/string/String.h>

namespace ply {

//------------------------------------------------------------------
// InStream
//------------------------------------------------------------------
PLY_NO_INLINE InStream::InStream(InStream&& other) : status{other.status} {
    this->curByte = other.curByte;
    this->endByte = other.endByte;
    this->startByte = other.startByte; // Assume all union members are same size
    this->reserved = other.reserved;   // Assume all union members are same size
    other.startByte = nullptr;
    other.curByte = nullptr;
    other.endByte = nullptr;
    other.reserved = nullptr;
    other.status = InStream::Status{InStream::Type::View, 0};
    other.status.eof = 1;
}

PLY_NO_INLINE InStream::InStream(OptionallyOwned<InPipe>&& inPipe, u32 chunkSizeExp)
    : status{Type::Pipe, chunkSizeExp} {
    PLY_ASSERT(inPipe);
    this->status.isPipeOwner = inPipe.isOwned() ? 1 : 0;
    this->inPipe = inPipe.release();
    // Init first chunk
    u32 chunkSize = this->getChunkSize();
    new (&this->chunk) Reference<ChunkListNode>{ChunkListNode::allocate(0, chunkSize)};
    this->curByte = this->chunk->bytes;
    this->endByte = this->chunk->bytes;
}

PLY_NO_INLINE void InStream::destructInternal() {
    PLY_ASSERT(this->status.type != (u32) Type::View);
    destruct(this->chunk);
    if (this->status.type == (u32) Type::Pipe) {
        if (this->status.isPipeOwner) {
            delete this->inPipe;
        }
    }
}

PLY_NO_INLINE u32 InStream::tryMakeBytesAvailableInternal(u32 numRequestedBytes) {
    // Function is allowed to make fewer bytes available in case of EOF/error, and the actual number
    // is returned.
    PLY_ASSERT(numRequestedBytes > 0);

    if (this->status.type == (u32) Type::View) {
        u32 numBytes = this->numBytesAvailable();
        if (numBytes == 0) {
            this->status.eof = 1;
        }
        return numBytes;
    }

    if (!this->chunk) {
        // Can reach here if InStream was moved from or readRemainingContents() was called
        PLY_ASSERT(this->status.eof);
        return 0;
    }

    Reference<ChunkListNode> tempChunk;
    u32 numBytesNeededToCompleteRequest = numRequestedBytes;
    for (;;) {
        // this->curByte points to the read position in the current chunk
        // this->endByte points to the write position in the current chunk
        PLY_ASSERT(this->chunk->viewUsedBytes().contains(this->curByte));
        PLY_ASSERT(this->endByte == this->chunk->bytes + this->chunk->writePos);

        u32 unreadDataInCurrentChunk =
            safeDemote<u32>(this->chunk->bytes + this->chunk->writePos - this->curByte);
        if (numBytesNeededToCompleteRequest <= unreadDataInCurrentChunk)
            break; // Current chunk has enough data to fulfill the remainder of the request.

        if (this->chunk->next) {
            // Another chunk follows this one in the list, and we already know, from the previous
            // check, that this chunk doesn't contain enough unread data to fulfill the rest of the
            // request.
            PLY_ASSERT(unreadDataInCurrentChunk < numBytesNeededToCompleteRequest);
            if (unreadDataInCurrentChunk > 0) {
                // Copy unread data to tempChunk (creating tempChunk first, if needed).
                if (!tempChunk) {
                    PLY_ASSERT(numBytesNeededToCompleteRequest == numRequestedBytes);
                    tempChunk = ChunkListNode::allocate(this->chunk->fileOffset +
                                                            (this->curByte - this->chunk->bytes),
                                                        numRequestedBytes);
                }
                if (tempChunk) {
                    memcpy(tempChunk->bytes + numRequestedBytes - numBytesNeededToCompleteRequest,
                           this->curByte, unreadDataInCurrentChunk);
                }
                numBytesNeededToCompleteRequest -= unreadDataInCurrentChunk;
            }
            // Advance to next chunk.
            u32 offsetIntoNextChunk = this->chunk->offsetIntoNextChunk;
            this->chunk = this->chunk->next;
            PLY_ASSERT(offsetIntoNextChunk <= this->chunk->writePos);
            this->curByte = this->chunk->bytes + offsetIntoNextChunk;
            this->endByte = this->chunk->bytes + this->chunk->writePos;
        } else {
            u32 unreadStorageInCurrentChunk = safeDemote<u32>(this->chunk->end() - this->curByte);
            if (unreadStorageInCurrentChunk < numBytesNeededToCompleteRequest) {
                // Append a new chunk
                PLY_ASSERT(this->chunk->offsetIntoNextChunk == 0);
                ChunkListNode::addChunkToTail(
                    this->chunk, max(numBytesNeededToCompleteRequest, this->getChunkSize()));
                this->curByte = this->chunk->bytes;
                this->endByte = this->chunk->bytes; // nothing written to this chunk yet
            }
            // Current chunk has enough storage to fulfill the remainder of the request.
            break;
        }
    }

    // Current chunk has enough storage to fulfill the remainder of the request, but we still might
    // need to fill it with data from the underlying pipe.
    PLY_ASSERT(numBytesNeededToCompleteRequest <=
               safeDemote<u32>(this->chunk->end() - this->curByte));
    u32 numBytesAvailableToReadInChunk;
    for (;;) {
        numBytesAvailableToReadInChunk =
            safeDemote<u32>((this->chunk->bytes + this->chunk->writePos) - this->curByte);
        if (numBytesAvailableToReadInChunk >= numBytesNeededToCompleteRequest)
            break; // Current chunk has enough data to fulfill remainder of request.

        PLY_ASSERT(!this->chunk->next); // We can only write to the end of the chunk list.
        // Read more data from the underlying pipe.
        u32 bytesRead = this->inPipe->readSome(
            MutableStringView::fromRange(this->chunk->bytes + this->chunk->writePos, this->chunk->end()));
        if (bytesRead == 0) {
            // We encountered EOF. As a safeguard/courtesy, pad memory with zeros up to the number
            // of needed bytes, even though the caller should **NOT** read any of these...
            memset(this->chunk->bytes + this->chunk->writePos, 0,
                   numBytesNeededToCompleteRequest - numBytesAvailableToReadInChunk);
            this->status.eof = 1;
            break;
        }
        this->chunk->writePos += bytesRead;
        this->endByte += bytesRead;
        PLY_ASSERT(this->endByte == this->chunk->bytes + this->chunk->writePos);
    }

    u32 numBytesToReturn =
        numRequestedBytes - numBytesNeededToCompleteRequest + numBytesAvailableToReadInChunk;

    // If we created a tempChunk, finish copying data to it now, and expose the tempChunk to the
    // caller.
    if (tempChunk) {
        PLY_ASSERT(numBytesNeededToCompleteRequest > 0);
        memcpy(tempChunk->bytes + numRequestedBytes - numBytesNeededToCompleteRequest,
               this->curByte, numBytesNeededToCompleteRequest);
        tempChunk->offsetIntoNextChunk =
            safeDemote<u32>((this->curByte + numBytesAvailableToReadInChunk) - this->chunk->bytes);
        PLY_ASSERT(tempChunk->offsetIntoNextChunk <= this->chunk->writePos);
        this->curByte = tempChunk->bytes;
        this->endByte = tempChunk->bytes + numBytesToReturn;
    }

    return numBytesToReturn;
}

PLY_NO_INLINE u64 InStream::getSeekPos() const {
    if (this->status.type == (u32) Type::View) {
        return safeDemote<u64>(this->curByte - this->startByte);
    } else {
        return this->chunk->fileOffset + safeDemote<u64>(this->curByte - this->chunk->bytes);
    }
}

PLY_NO_INLINE ChunkCursor InStream::getCursor() const {
    // View is not supported yet, but could be in the future:
    PLY_ASSERT(this->status.type != (u32) Type::View);
    PLY_ASSERT(!this->chunk || this->chunk->viewUsedBytes().contains(this->curByte));
    return {this->chunk, const_cast<char*>(this->curByte)};
}

PLY_NO_INLINE void InStream::rewind(ChunkCursor cursor) {
    // View is not supported yet, but could be in the future:
    PLY_ASSERT(this->status.type != (u32) Type::View);
    PLY_ASSERT(cursor.chunk->viewUsedBytes().contains(cursor.curByte));
    this->chunk = cursor.chunk;
    this->curByte = cursor.curByte;
    this->endByte = cursor.chunk->bytes + cursor.chunk->writePos;
    this->status.eof = cursor.chunk ? 0 : 1;
}

PLY_NO_INLINE bool InStream::readSlowPath(MutableStringView dst) {
    if (this->status.eof)
        return false;

    while (dst.numBytes > 0) {
        if (!this->tryMakeBytesAvailable()) {
            memset(dst.bytes, 0, dst.numBytes);
            return false;
        }
        u32 toCopy = min<u32>(dst.numBytes, this->numBytesAvailable());
        memcpy(dst.bytes, this->curByte, toCopy);
        this->curByte += toCopy;
        dst.offsetHead(toCopy);
    }

    return true;
}

PLY_NO_INLINE String InStream::readRemainingContents() {
    ChunkCursor startChunk = this->getCursor();
    while (this->tryMakeBytesAvailable()) {
        this->curByte = this->endByte;
    }
    PLY_ASSERT(this->status.eof);
    this->chunk.clear();
    this->curByte = nullptr;
    this->endByte = nullptr;
    return ChunkCursor::toString(std::move(startChunk));
}

} // namespace ply

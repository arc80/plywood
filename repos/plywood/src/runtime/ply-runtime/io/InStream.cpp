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

PLY_NO_INLINE InStream::InStream(OptionallyOwned<InPipe>&& inPipe, u32 blockSizeExp)
    : status{Type::Pipe, blockSizeExp} {
    PLY_ASSERT(inPipe);
    this->status.isPipeOwner = inPipe.isOwned() ? 1 : 0;
    this->inPipe = inPipe.release();
    // Init first block
    u32 blockSize = this->getBlockSize();
    new (&this->block) Reference<BlockList::Footer>{BlockList::createBlock(blockSize)};
    this->curByte = this->block->bytes;
    this->endByte = this->block->bytes;
}

PLY_NO_INLINE void InStream::destructInternal() {
    PLY_ASSERT(this->status.type != (u32) Type::View);
    destruct(this->block);
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

    if (!this->block) {
        // Can reach here if InStream was moved from or readRemainingContents() was called
        PLY_ASSERT(this->status.eof);
        return 0;
    }

    Reference<BlockList::Footer> overlayBlock;
    u32 numBytesNeededToCompleteRequest = numRequestedBytes;
    for (;;) {
        // this->curByte points to the read position in the current block.
        // this->endByte points to the write position in the current block.
        PLY_ASSERT(this->block->viewUsedBytes().contains(this->curByte));
        PLY_ASSERT(this->endByte == this->block->end());

        u32 numUnreadBytesInCurrentBlock = safeDemote<u32>(this->block->end() - this->curByte);
        if (numBytesNeededToCompleteRequest <= numUnreadBytesInCurrentBlock)
            break; // Current block has enough data to fulfill the remainder of the request.

        if (this->block->nextBlock) {
            // Another block follows this one in the list, and we already know, from the previous
            // check, that this block doesn't contain enough unread data to fulfill the rest of the
            // request.
            PLY_ASSERT(numUnreadBytesInCurrentBlock < numBytesNeededToCompleteRequest);
            if (numUnreadBytesInCurrentBlock > 0) {
                // Copy unread data to overlayBlock (creating overlayBlock first, if needed).
                if (!overlayBlock) {
                    PLY_ASSERT(numBytesNeededToCompleteRequest == numRequestedBytes);
                    overlayBlock = BlockList::createOverlayBlock({this->block, this->block->bytes},
                                                                 numRequestedBytes);
                }
                if (overlayBlock) {
                    memcpy(overlayBlock->bytes + numRequestedBytes -
                               numBytesNeededToCompleteRequest,
                           this->curByte, numUnreadBytesInCurrentBlock);
                }
                numBytesNeededToCompleteRequest -= numUnreadBytesInCurrentBlock;
            }
            // Advance to next block.
            BlockList::Footer* nextBlock = this->block->nextBlock;
            u32 offsetIntoNextBlock =
                this->block->numBytesUsed -
                safeDemote<u32>(nextBlock->fileOffset - this->block->fileOffset);
            this->block = nextBlock;
            PLY_ASSERT(offsetIntoNextBlock <= nextBlock->numBytesUsed);
            this->curByte = nextBlock->bytes + offsetIntoNextBlock;
            this->endByte = nextBlock->bytes + nextBlock->numBytesUsed;
        } else {
            u32 unreadStorageInCurrentBlock = safeDemote<u32>(this->block->end() - this->curByte);
            if (unreadStorageInCurrentBlock < numBytesNeededToCompleteRequest) {
                // Append a new block
                BlockList::appendBlock(this->block, max(numBytesNeededToCompleteRequest, this->getBlockSize()));
                this->curByte = this->block->bytes;
                this->endByte = this->block->bytes; // nothing written to this block yet
            }
            // Current block has enough storage to fulfill the remainder of the request.
            break;
        }
    }

    // Current block has enough storage to fulfill the remainder of the request, but we still might
    // need to fill it with data from the underlying pipe.
    PLY_ASSERT(numBytesNeededToCompleteRequest <=
               safeDemote<u32>(this->block->end() - this->curByte));
    u32 numBytesAvailableToReadInBlock;
    for (;;) {
        numBytesAvailableToReadInBlock =
            safeDemote<u32>((this->block->bytes + this->block->numBytesUsed) - this->curByte);
        if (numBytesAvailableToReadInBlock >= numBytesNeededToCompleteRequest)
            break; // Current block has enough data to fulfill remainder of request.

        PLY_ASSERT(!this->block->nextBlock); // We can only write to the end of the block list.
        // Read more data from the underlying pipe.
        u32 bytesRead = this->inPipe->readSome(MutableStringView::fromRange(
            this->block->bytes + this->block->numBytesUsed, this->block->end()));
        if (bytesRead == 0) {
            // We encountered EOF. As a safeguard/courtesy, pad memory with zeros up to the number
            // of needed bytes, even though the caller should **NOT** read any of these...
            memset(this->block->bytes + this->block->numBytesUsed, 0,
                   numBytesNeededToCompleteRequest - numBytesAvailableToReadInBlock);
            this->status.eof = 1;
            break;
        }
        this->block->numBytesUsed += bytesRead;
        this->endByte += bytesRead;
        PLY_ASSERT(this->endByte == this->block->bytes + this->block->numBytesUsed);
    }

    u32 numBytesToReturn =
        numRequestedBytes - numBytesNeededToCompleteRequest + numBytesAvailableToReadInBlock;

    // If we created a overlayBlock, finish copying data to it now, and expose the overlayBlock to
    // the caller.
    if (overlayBlock) {
        PLY_ASSERT(numBytesNeededToCompleteRequest > 0);
        memcpy(overlayBlock->bytes + numRequestedBytes - numBytesNeededToCompleteRequest,
               this->curByte, numBytesNeededToCompleteRequest);
        this->curByte = overlayBlock->bytes;
        this->endByte = overlayBlock->bytes + numBytesToReturn;
    }

    return numBytesToReturn;
}

PLY_NO_INLINE u64 InStream::getSeekPos() const {
    if (this->status.type == (u32) Type::View) {
        return safeDemote<u64>(this->curByte - this->startByte);
    } else {
        return this->block->fileOffset + safeDemote<u64>(this->curByte - this->block->bytes);
    }
}

PLY_NO_INLINE BlockList::Ref InStream::getBlockRef() const {
    // View is not supported yet, but could be in the future:
    PLY_ASSERT(this->status.type != (u32) Type::View);
    PLY_ASSERT(!this->block || this->block->viewUsedBytes().contains(this->curByte));
    return {this->block, const_cast<char*>(this->curByte)};
}

PLY_NO_INLINE void InStream::rewind(const BlockList::WeakRef& pos) {
    // View is not supported yet, but could be in the future:
    PLY_ASSERT(this->status.type != (u32) Type::View);
    PLY_ASSERT(pos.block->viewUsedBytes().contains(pos.byte));
    this->block = pos.block;
    this->curByte = pos.byte;
    this->endByte = pos.block->bytes + pos.block->numBytesUsed;
    this->status.eof = pos.block ? 0 : 1;
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

PLY_NO_INLINE bool InStream::skipSlowPath(u32 numBytes) {
    if (this->status.eof)
        return false;

    while (numBytes > 0) {
        if (!this->tryMakeBytesAvailable()) {
            return false;
        }
        u32 toSkip = min<u32>(numBytes, this->numBytesAvailable());
        this->curByte += toSkip;
        numBytes -= toSkip;
    }

    return true;
}

PLY_NO_INLINE String InStream::readRemainingContents() {
    BlockList::Ref startPos = this->getBlockRef();
    while (this->tryMakeBytesAvailable()) {
        this->curByte = this->endByte;
    }
    PLY_ASSERT(this->status.eof);
    this->block.clear();
    this->curByte = nullptr;
    this->endByte = nullptr;
    return BlockList::toString(std::move(startPos));
}

} // namespace ply

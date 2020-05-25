/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-runtime/Precomp.h>
#include <ply-runtime/container/ChunkList.h>
#include <ply-runtime/memory/Heap.h>
#include <ply-runtime/io/OutStream.h>

namespace ply {

//-------------------------------------------------
// ChunkListNode
//-------------------------------------------------
PLY_NO_INLINE Reference<ChunkListNode> ChunkListNode::allocate(u64 fileOffset, u32 numBytes) {
    u32 alignment = ChunkListNode::DefaultAlignment;
    u32 alignedNumBytes = alignPowerOf2(numBytes, alignment);
    u32 allocSize = alignedNumBytes + ChunkListNode::getAlignedSize();
    u8* bytes = (u8*) PLY_HEAP.allocAligned(allocSize, alignment);
    ChunkListNode* chunk = (ChunkListNode*) (bytes + alignedNumBytes);
    new (chunk) ChunkListNode;
    chunk->fileOffset = fileOffset;
    chunk->bytes = bytes;
    chunk->numBytes = alignedNumBytes;
    return chunk;
}

PLY_NO_INLINE void ChunkListNode::addChunkToTail(Reference<ChunkListNode>& chunkRef, u32 numBytes) {
    PLY_ASSERT(chunkRef);
    PLY_ASSERT(!chunkRef->next);
    PLY_ASSERT(chunkRef->offsetIntoNextChunk == 0);
    PLY_ASSERT(chunkRef->refCount > 0);
    if (chunkRef->refCount == 1 && chunkRef->numBytes <= numBytes) {
        chunkRef->fileOffset += chunkRef->writePos;
        chunkRef->writePos = 0;
    } else {
        Reference<ChunkListNode> next =
            ChunkListNode::allocate(chunkRef->fileOffset + chunkRef->writePos, numBytes);
        chunkRef->next = next;
        chunkRef = next;
    }
}

PLY_NO_INLINE void ChunkListNode::onRefCountZero() {
    ChunkListNode* n = this->next;
    PLY_HEAP.freeAligned(this->bytes); // frees the header too
    if (n) {
        n->decRef();
    }
}

PLY_NO_INLINE void ChunkCursor::iterateOverViews(LambdaView<void(ConstBufferView)> callback,
                                                 const ChunkCursor& end) const {
    PLY_ASSERT(this->curByte);
    if (this->chunk == end.chunk) {
        PLY_ASSERT(this->curByte <= end.curByte);
        PLY_ASSERT(end.curByte);
        callback(ConstBufferView::fromRange(this->curByte, end.curByte));
    } else {
        PLY_ASSERT(this->chunk->viewUsedBytes().contains(this->curByte));
        callback(
            ConstBufferView::fromRange(this->curByte, this->chunk->bytes + this->chunk->numBytes));
        const ChunkListNode* chunk = this->chunk->next;
        while (chunk != end.chunk) {
            callback(chunk->viewUsedBytes());
            PLY_ASSERT(chunk->next || !end.chunk);
            chunk = chunk->next;
        }
        if (end.chunk) {
            PLY_ASSERT(end.chunk->viewUsedBytes().contains(end.curByte));
            callback(ConstBufferView::fromRange(end.chunk->bytes, end.curByte));
        }
    }
}

PLY_NO_INLINE Buffer ChunkCursor::toBuffer(ChunkCursor&& start, const ChunkCursor& end) {
    PLY_ASSERT(start.curByte);

    if (!start.chunk->next && start.chunk->refCount == 1 && start.chunk->bytes == start.curByte) {
        // Optimization: When there is only one chunk and only one reference to the chunk, we can
        // reuse its memory, and no memcpy is performed:
        u32 numBytes = start.chunk->writePos;
        u8* bytes = (u8*) PLY_HEAP.realloc(start.curByte, numBytes);
        Buffer result = Buffer::adopt(bytes, numBytes);
        start.chunk.release();
        start.curByte = nullptr;
        return result;
    }

    u32 numBytes = 0;
    start.iterateOverViews(
        [&](ConstBufferView view) {
            u32 sum = numBytes + view.numBytes;
            PLY_ASSERT(sum >= numBytes); // overflow check
            numBytes = sum;
        },
        end);
    Buffer result = Buffer::allocate(numBytes);
    u32 offset = 0;
    start.iterateOverViews(
        [&](ConstBufferView view) {
            memcpy(result.bytes + offset, view.bytes, view.numBytes);
            offset += view.numBytes;
        },
        end);
    return result;
}

PLY_NO_INLINE void ChunkCursor::writeToStream(OutStream* outs, const ChunkCursor& end) const {
    this->iterateOverViews(
        [&](ConstBufferView view) { //
            outs->write(view);
        },
        end);
}

PLY_NO_INLINE void ChunkCursor::advanceToNextChunk() {
    PLY_ASSERT(this->curByte >= this->chunk->bytes + this->chunk->writePos);
    for (;;) {
        ChunkListNode* nextChunk = this->chunk->next;
        if (nextChunk) {
            this->curByte = nextChunk->bytes + this->chunk->offsetIntoNextChunk;
            this->chunk = nextChunk;
            if (uptr(this->curByte - nextChunk->bytes) < nextChunk->writePos)
                break;
        } else {
            this->chunk = nullptr;
            this->curByte = nullptr;
            break;
        }
    }
}

} // namespace ply

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
    u32 alignedNumBytes = alignPowerOf2(numBytes, (u32) alignof(ChunkListNode));
    u32 allocSize = alignedNumBytes + sizeof(ChunkListNode);
    char* bytes = (char*) PLY_HEAP.alloc(allocSize);
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

PLY_NO_INLINE void ChunkListNode::decRef() {
    ChunkListNode* node = this;
    while (node) {
        node->refCount--;
        PLY_ASSERT(node->refCount >= 0);
        if (node->refCount > 0)
            break;
        ChunkListNode* n = node->next;
        PLY_HEAP.free(node->bytes);
        node = n;
    }
}

PLY_NO_INLINE void ChunkCursor::iterateOverViews(LambdaView<void(StringView)> callback,
                                                 const ChunkCursor& end) const {
    PLY_ASSERT(this->curByte);
    if (this->chunk == end.chunk) {
        PLY_ASSERT(this->curByte <= end.curByte);
        PLY_ASSERT(end.curByte);
        callback(StringView::fromRange(this->curByte, end.curByte));
    } else {
        PLY_ASSERT(this->chunk->viewUsedBytes().contains(this->curByte));
        callback(StringView::fromRange(this->curByte, this->chunk->bytes + this->chunk->writePos));
        const ChunkListNode* chunk = this->chunk->next;
        while (chunk != end.chunk) {
            callback(chunk->viewUsedBytes());
            PLY_ASSERT(chunk->next || !end.chunk);
            chunk = chunk->next;
        }
        if (end.chunk) {
            PLY_ASSERT(end.chunk->viewUsedBytes().contains(end.curByte));
            callback(StringView::fromRange(end.chunk->bytes, end.curByte));
        }
    }
}

PLY_NO_INLINE String ChunkCursor::toString(ChunkCursor&& start, const ChunkCursor& end) {
    PLY_ASSERT(start.curByte);

    if (!start.chunk->next && start.chunk->refCount == 1 && start.chunk->bytes == start.curByte) {
        // Optimization: When there is only one chunk and only one reference to the chunk, we can
        // reuse its memory, and no memcpy is performed:
        u32 numBytes = start.chunk->writePos;
        char* bytes = (char*) PLY_HEAP.realloc(start.curByte, numBytes);
        String result = String::adopt(bytes, numBytes);
        start.chunk.release();
        start.curByte = nullptr;
        return result;
    }

    u32 numBytes = 0;
    start.iterateOverViews(
        [&](StringView view) {
            u32 sum = numBytes + view.numBytes;
            PLY_ASSERT(sum >= numBytes); // overflow check
            numBytes = sum;
        },
        end);
    String result = String::allocate(numBytes);
    u32 offset = 0;
    start.iterateOverViews(
        [&](StringView view) {
            memcpy(result.bytes + offset, view.bytes, view.numBytes);
            offset += view.numBytes;
        },
        end);
    return result;
}

PLY_NO_INLINE void ChunkCursor::writeToStream(OutStream* outs, const ChunkCursor& end) const {
    this->iterateOverViews(
        [&](StringView view) { //
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

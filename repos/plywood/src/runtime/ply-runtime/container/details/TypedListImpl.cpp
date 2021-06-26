/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/

#include <ply-runtime/Precomp.h>
#include <ply-runtime/container/details/TypedListImpl.h>

namespace ply {
namespace details {

PLY_NO_INLINE TypedListImpl::TypedListImpl() {
    u32 chunkSize = ChunkListNode::DefaultChunkSize; // FIXME make configurable?
    this->tailChunk = ChunkListNode::allocate(0, chunkSize);
    this->headChunk = this->tailChunk;
}

PLY_NO_INLINE TypedListImpl::TypedListImpl(TypedListImpl&& other)
    : tailChunk{std::move(other.tailChunk)}, headChunk{std::move(other.headChunk)} {
}

PLY_NO_INLINE TypedListImpl::~TypedListImpl() {
}

PLY_NO_INLINE void TypedListImpl::operator=(TypedListImpl&& other) {
    this->tailChunk = std::move(other.tailChunk);
    this->headChunk = std::move(other.headChunk);
}

PLY_NO_INLINE char* TypedListImpl::beginWriteInternal(u32 numRequestedBytes) {
    PLY_ASSERT(this->tailChunk->numRemainingBytes() < numRequestedBytes);

    // Get a new chunk to write to
    u32 minChunkSize = ChunkListNode::DefaultChunkSize; // FIXME make configurable?
    ChunkListNode::addChunkToTail(this->tailChunk, max(numRequestedBytes, minChunkSize));
    return this->tailChunk->bytes;
}

PLY_NO_INLINE String TypedListImpl::moveToString() {
    char* bytes = this->headChunk->bytes;
    String result = ChunkCursor::toString({std::move(this->headChunk), bytes});
    PLY_ASSERT(this->headChunk == nullptr);
    this->tailChunk = nullptr;
    return result;
}

PLY_NO_INLINE void destructTypedListInternal(ChunkListNode* chunk, void (*dtor)(void*),
                                               u32 itemSize) {
    while (chunk) {
        char* curItem = chunk->bytes;
        char* endItem = curItem + chunk->writePos;
        while (curItem < endItem) {
            dtor(curItem);
            curItem += itemSize;
        }
        PLY_ASSERT(curItem == endItem);
        chunk = chunk->next;
    }
}

PLY_NO_INLINE TypedListReaderImpl::TypedListReaderImpl(const TypedListImpl& tb)
    : chunk{tb.headChunk} {
    this->curByte = this->chunk->bytes;
    this->endByte = this->chunk->curByte();
}

PLY_NO_INLINE u32 TypedListReaderImpl::tryMakeBytesAvailableInternal(u32 numRequestedBytes) {
    // Could we factor out common code between this and InStream::tryMakeBytesAvailableInternal?
    PLY_ASSERT(numRequestedBytes > 0);
    // For now, we assume chunks were written by a TypedListWriter. We don't do splicing like
    // InStream::tryMakeBytesAvailableInternal:
    PLY_ASSERT(this->numBytesAvailable() == 0);
    if (!this->chunk->next)
        return 0;

    // Again, we assume chunks were written by a TypedListWriter:
    PLY_ASSERT(this->chunk->offsetIntoNextChunk == 0);
    this->chunk = this->chunk->next;
    PLY_ASSERT(this->chunk->writePos >= numRequestedBytes);
    this->curByte = this->chunk->bytes;
    this->endByte = this->chunk->bytes + this->chunk->writePos;
    return this->chunk->writePos;
}

} // namespace details
} // namespace ply

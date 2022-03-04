/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/

#include <ply-runtime/Precomp.h>
#include <ply-runtime/container/details/TypedListImpl.h>

namespace ply {
namespace details {

PLY_NO_INLINE TypedListImpl::TypedListImpl() {
    u32 chunkSize = BlockList::DefaultBlockSize;
    this->head = BlockList::createBlock(chunkSize);
    this->tail = this->head;
}

PLY_NO_INLINE TypedListImpl::TypedListImpl(TypedListImpl&& other)
    : head{std::move(other.head)}, tail{other.tail} {
}

PLY_NO_INLINE TypedListImpl::~TypedListImpl() {
}

PLY_NO_INLINE void TypedListImpl::operator=(TypedListImpl&& other) {
    this->head = std::move(other.head);
    this->tail = other.tail;
}

PLY_NO_INLINE char* TypedListImpl::beginWriteInternal(u32 numRequestedBytes) {
    // numRequestedBytes should not fit inside the remaining space of the current block.
    PLY_ASSERT(numRequestedBytes > (this->tail->blockSize - this->tail->numBytesUsed));

    // Get a new chunk to write to.
    u32 minChunkSize = BlockList::DefaultBlockSize;
    this->tail = BlockList::appendBlock(this->tail, max(numRequestedBytes, minChunkSize));
    return this->tail->bytes;
}

PLY_NO_INLINE String TypedListImpl::moveToString() {
    char *startByte = this->head->bytes;
    String result = BlockList::toString({std::move(this->head), startByte});
    this->head.clear();
    this->tail = nullptr;
    return result;
}

PLY_NO_INLINE void destructTypedListInternal(BlockList::Footer* block, void (*dtor)(void*),
                                             u32 itemSize) {
    while (block) {
        char* curItem = block->bytes;
        char* endItem = curItem + block->numBytesUsed;
        while (curItem < endItem) {
            dtor(curItem);
            curItem += itemSize;
        }
        PLY_ASSERT(curItem == endItem);
        block = block->nextBlock;
    }
}

PLY_NO_INLINE TypedListReaderImpl::TypedListReaderImpl(const TypedListImpl& tb) : block{tb.head} {
    this->curByte = this->block->bytes;
    this->endByte = this->block->unused();
}

PLY_NO_INLINE u32 TypedListReaderImpl::tryMakeBytesAvailableInternal(u32 numRequestedBytes) {
    // Could we factor out common code between this and InStream::tryMakeBytesAvailableInternal?
    PLY_ASSERT(numRequestedBytes > 0);

    // For now, we assume chunks were written by a TypedListWriter. We don't do splicing like
    // InStream::tryMakeBytesAvailableInternal:
    PLY_ASSERT(this->numBytesAvailable() == 0);
    if (!this->block->nextBlock)
        return 0;

    // Again, we assume blocks were written by a TypedListWriter.
    this->block = this->block->nextBlock;
    PLY_ASSERT(this->block->numBytesUsed >=
               numRequestedBytes); // This is the line that relies on the assumption.
    this->curByte = this->block->bytes;
    this->endByte = this->block->bytes + this->block->numBytesUsed;
    return this->block->numBytesUsed;
}

} // namespace details
} // namespace ply

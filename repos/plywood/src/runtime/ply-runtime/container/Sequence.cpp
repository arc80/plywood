/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-runtime/Precomp.h>
#include <ply-runtime/container/Sequence.h>

namespace ply {
namespace details {

PLY_NO_INLINE void destructSequence(Reference<BlockList::Footer>* headRef,
                                    void (*destructViewAs)(StringView)) {
    BlockList::Footer* blockToFree = headRef->release();
    while (blockToFree) {
        // There should be only own reference: the Sequence.
        PLY_ASSERT(blockToFree->refCount == 1);
        destructViewAs(blockToFree->viewUsedBytes());
        BlockList::Footer* nextBlock = blockToFree->nextBlock.release();
        // The destructor of this->nextBlock is now trivial, so we can skip it, and when we free the
        // block data from the heap, it also frees the footer.
        PLY_HEAP.free(blockToFree->bytes);
        blockToFree = nextBlock;
    }
}

PLY_NO_INLINE void beginWriteInternal(BlockList::Footer** tail, u32 numBytes) {
    PLY_ASSERT((*tail)->viewUnusedBytes().numBytes < numBytes);
    *tail = BlockList::appendBlock(*tail, max(BlockList::DefaultBlockSize, numBytes));
}

PLY_NO_INLINE void popTail(BlockList::Footer** tail, u32 numBytes, void (*destructViewAs)(StringView)) {
    BlockList::Footer* block = *tail;
    while (numBytes > 0) {
        u32 bytesToPop = min(numBytes, block->viewUsedBytes().numBytes);
        // It is illegal to attempt to pop more items than the sequence contains.
        PLY_ASSERT(bytesToPop > 0);
        destructViewAs({block->unused() - bytesToPop, bytesToPop});
        block->numBytesUsed -= bytesToPop;
        numBytes -= bytesToPop;
        if (block->viewUsedBytes().numBytes > 0) {
            PLY_ASSERT(numBytes == 0);
            break;
        }
        block = block->prevBlock;
        *tail = block;
        block->nextBlock.clear();
    }
}

PLY_NO_INLINE u32 getTotalNumBytes(BlockList::Footer* head) {
    u32 numBytes = 0;
    while (head) {
        numBytes += head->viewUsedBytes().numBytes;
        head = head->nextBlock;
    }
    return numBytes;
}

PLY_NO_INLINE char* read(BlockList::WeakRef* weakRef, u32 itemSize) {
    sptr numBytesAvailable = weakRef->block->unused() - weakRef->byte;
    // It's illegal to call this function at the end of a sequence.
    PLY_ASSERT(numBytesAvailable >= itemSize);
    char* result = weakRef->byte;
    weakRef->byte += itemSize;
    numBytesAvailable -= itemSize;
    if (numBytesAvailable == 0) {
        numBytesAvailable = BlockList::jumpToNextBlock(weakRef);
        // We might now be at the end of the sequence.
    } else {
        // numBytesAvailable should always be a multiple of the item size.
        PLY_ASSERT(numBytesAvailable >= itemSize);
    }
    return result;
}

} // namespace details
} // namespace ply

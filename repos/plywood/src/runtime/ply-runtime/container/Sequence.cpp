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

} // namespace details
} // namespace ply

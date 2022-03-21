/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-runtime/Precomp.h>
#include <ply-runtime/container/BlockList.h>
#include <ply-runtime/memory/Heap.h>
#include <ply-runtime/io/OutStream.h>

namespace ply {

//--------------------------------------
// BlockList::Footer
//--------------------------------------
PLY_NO_INLINE void BlockList::Footer::onRefCountZero() {
    BlockList::Footer* blockToFree = this;
    while (blockToFree) {
        BlockList::Footer* nextBlock = blockToFree->nextBlock.release();
        // The destructor of this->nextBlock is now trivial, so we can skip it, and when we free the
        // block data from the heap, it also frees the footer.
        PLY_HEAP.free(blockToFree->bytes);
        if (nextBlock) {
            nextBlock->prevBlock = nullptr;
            PLY_ASSERT(nextBlock->refCount > 0);
            if (--nextBlock->refCount != 0)
                break;
        }
        blockToFree = nextBlock;
    }
}

PLY_NO_INLINE BlockList::WeakRef BlockList::Footer::weakRefToNext() const {
    BlockList::WeakRef result;
    result.block = this->nextBlock;
    if (result.block) {
        u32 fileDelta = safeDemote<u32>((s64) result.block->fileOffset - this->fileOffset);
        PLY_ASSERT(fileDelta <= this->numBytesUsed);
        result.byte = result.block->bytes + (this->numBytesUsed - fileDelta);
        PLY_ASSERT(result.block->viewUsedBytes().contains(result.byte));
    }
    return result;
}

//-------------------------------------------------
// BlockList static member functions
//-------------------------------------------------
PLY_NO_INLINE Reference<BlockList::Footer> BlockList::createBlock(u32 numBytes) {
    PLY_ASSERT(numBytes > 100);
    u32 alignedNumBytes = alignPowerOf2(numBytes, (u32) alignof(BlockList::Footer));
    u32 allocSize = alignedNumBytes + sizeof(BlockList::Footer);
    char* bytes = (char*) PLY_HEAP.alloc(allocSize);
    BlockList::Footer* block = (BlockList::Footer*) (bytes + alignedNumBytes);
    new (block) BlockList::Footer; // Construct in-place
    block->bytes = bytes;
    block->blockSize = numBytes;
    return block;
}

PLY_DLL_ENTRY Reference<BlockList::Footer> BlockList::createOverlayBlock(const WeakRef& pos,
                                                                         u32 numBytes) {
    Reference<Footer> newBlock = createBlock(numBytes);
    newBlock->fileOffset = pos.block->fileOffset + pos.block->offsetOf(pos.byte);
    newBlock->nextBlock = pos.block->nextBlock;
    return newBlock;
}

PLY_NO_INLINE BlockList::Footer* BlockList::appendBlock(Footer* block, u32 numBytes) {
    PLY_ASSERT(block->nextBlock.isEmpty());

    block->nextBlock = createBlock(numBytes);
    block->nextBlock->fileOffset = block->fileOffset + block->numBytesUsed;
    block->nextBlock->prevBlock = block;
    return block->nextBlock;
}

PLY_NO_INLINE void BlockList::appendBlockWithRecycle(Reference<Footer>& block, u32 numBytes) {
    PLY_ASSERT(block->nextBlock.isEmpty());

    // When possible, just reuse the existing block.
    if (block->refCount == 1 && block->blockSize == numBytes) {
        block->fileOffset += block->numBytesUsed;
        block->numBytesUsed = 0;
        return;
    }

    Reference<Footer> newBlock = createBlock(numBytes);
    block->nextBlock = newBlock;
    newBlock->fileOffset = block->fileOffset + block->numBytesUsed;
    newBlock->prevBlock = block;
    block = std::move(newBlock);
}

PLY_NO_INLINE u32 BlockList::jumpToNextBlock(WeakRef* weakRef) {
    PLY_ASSERT(weakRef->byte = weakRef->block->unused());
    if (!weakRef->block->nextBlock)
        return 0;
    weakRef->block = weakRef->block->nextBlock;
    weakRef->byte = weakRef->block->start();
    return weakRef->block->viewUsedBytes().numBytes;
}

PLY_NO_INLINE u32 BlockList::jumpToPrevBlock(WeakRef* weakRef) {
    PLY_ASSERT(weakRef->byte = weakRef->block->start());
    if (!weakRef->block->prevBlock)
        return 0;
    weakRef->block = weakRef->block->prevBlock;
    weakRef->byte = weakRef->block->unused();
    return weakRef->block->viewUsedBytes().numBytes;
}

PLY_NO_INLINE String BlockList::toString(Ref&& start, const WeakRef& end) {
    PLY_ASSERT(start.byte);

    // Check for special case: When there is only one block and only one reference to the block,
    // we can truncate this block and return it as a String directly.
    if (!start.block->nextBlock && start.block->refCount == 1 && start.block->bytes == start.byte) {
        u32 numBytes = start.block->numBytesUsed;
        char* bytes = (char*) PLY_HEAP.realloc(start.byte, numBytes);
        String result = String::adopt(bytes, numBytes);
        start.block.release();
        start.byte = nullptr;
        return result;
    }

    // Count the total number of bytes.
    u32 numBytes = 0;
    for (StringView view : iterateOverViews(start, end)) {
        u32 sum = numBytes + view.numBytes;
        PLY_ASSERT(sum >= numBytes); // overflow check
        numBytes = sum;
    }

    // Allocate a new String and copy data into it.
    String result = String::allocate(numBytes);
    u32 offset = 0;
    for (StringView view : iterateOverViews(start, end)) {
        memcpy(result.bytes + offset, view.bytes, view.numBytes);
        offset += view.numBytes;
    }
    return result;
}

//--------------------------------------
// BlockList object
//--------------------------------------
PLY_NO_INLINE BlockList::BlockList() {
    this->head = BlockList::createBlock();
    this->tail = this->head;
}

PLY_NO_INLINE BlockList::~BlockList() {
}

PLY_NO_INLINE char* BlockList::appendBytes(u32 numBytes) {
    if (this->tail->viewUnusedBytes().numBytes < numBytes) {
        this->tail = BlockList::appendBlock(this->tail, max(numBytes, BlockList::DefaultBlockSize));
        PLY_ASSERT(this->tail->viewUnusedBytes().numBytes >= numBytes);
    }
    char* result = this->tail->unused();
    this->tail->numBytesUsed += numBytes;
    return result;
}

PLY_NO_INLINE void BlockList::popLastBytes(u32 numBytes) {
    PLY_ASSERT(this->tail->numBytesUsed >= numBytes);
    this->tail->numBytesUsed -= numBytes;
    if ((this->tail->numBytesUsed == 0) && (this->head != this->tail)) {
        this->tail = this->tail->prevBlock;
        this->tail->nextBlock.clear();
    }
}

} // namespace ply

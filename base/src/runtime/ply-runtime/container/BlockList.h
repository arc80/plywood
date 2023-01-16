/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/container/Reference.h>
#include <ply-runtime/string/String.h>

namespace ply {

struct OutStream;

struct BlockList {
    static const u32 DefaultBlockSize = 2048;

    struct WeakRef;

    //--------------------------------------
    // Blocks are allocated on the heap, and the block footer is located contiguously in memory
    // immediately following the block data.
    //--------------------------------------
    struct Footer {
        u64 fileOffset = 0; // Total number of bytes used in all preceding blocks
        Reference<Footer> nextBlock;
        Footer* prevBlock = nullptr;
        char* bytes = nullptr;
        u32 startOffset = 0;
        u32 numBytesUsed = 0; // Number of bytes in this block that contain valid data
        u32 blockSize = 0;    // Total number of bytes allocated for this block
        mutable s32 refCount = 0;

        PLY_DLL_ENTRY void onRefCountZero();
        PLY_INLINE void incRef() {
            this->refCount++;
        }
        PLY_INLINE void decRef() {
            PLY_ASSERT(this->refCount > 0);
            if (--this->refCount == 0) {
                this->onRefCountZero();
            }
        }
        PLY_INLINE char* start() const {
            return this->bytes + this->startOffset;
        }
        PLY_INLINE char* unused() const {
            return this->bytes + this->numBytesUsed;
        }
        PLY_INLINE char* end() const {
            return this->bytes + this->blockSize;
        }
        PLY_INLINE u32 offsetOf(char* byte) {
            uptr ofs = byte - this->bytes;
            PLY_ASSERT(ofs <= this->numBytesUsed);
            return (u32) ofs;
        }
        PLY_INLINE StringView viewUsedBytes() const {
            return {this->start(), this->numBytesUsed - this->startOffset};
        }
        PLY_INLINE MutStringView viewUnusedBytes() {
            return {this->unused(), this->blockSize - this->numBytesUsed};
        }
        // Returns a WeakRef to next block, and the next byte after the last byte in this block,
        // taking the difference in fileOffsets into account since it's possible for adjacent blocks
        // to overlap.
        PLY_DLL_ENTRY WeakRef weakRefToNext() const;
    };

    //--------------------------------------
    // WeakRef
    //--------------------------------------
    struct WeakRef {
        Footer* block = nullptr;
        char* byte = nullptr;

        PLY_INLINE WeakRef() {
        }
        PLY_INLINE WeakRef(const WeakRef& other) : block{other.block}, byte{other.byte} {
        }
        PLY_INLINE WeakRef(Footer* block, char* byte) : block{block}, byte{byte} {
            PLY_ASSERT(uptr(byte - block->bytes) <= block->blockSize);
        }
        PLY_INLINE WeakRef(Footer* block, u32 offset) : block{block}, byte{block->bytes + offset} {
            PLY_ASSERT(offset <= block->blockSize);
        }
        PLY_INLINE void operator=(const WeakRef& other) {
            this->block = other.block;
            this->byte = other.byte;
        }
        PLY_INLINE bool operator!=(const WeakRef& other) const {
            return this->block != other.block || this->byte != other.byte;
        }
        PLY_INLINE WeakRef normalized() const {
            if (this->block->nextBlock && (this->byte == this->block->unused())) {
                return {this->block->nextBlock, this->block->start()};
            }
            return *this;
        }
    };

    //--------------------------------------
    // Ref
    //--------------------------------------
    struct Ref {
        Reference<Footer> block;
        char* byte = nullptr;

        PLY_INLINE Ref() {
        }
        PLY_INLINE Ref(const Ref& other) : block{other.block}, byte{other.byte} {
        }
        PLY_INLINE Ref(Ref&& other) : block{std::move(other.block)}, byte{other.byte} {
        }
        PLY_INLINE Ref(Footer* block) : block{block}, byte{block->bytes} {
        }
        PLY_INLINE Ref(Footer* block, char* byte) : block{block}, byte{byte} {
            PLY_ASSERT(uptr(byte - block->bytes) <= block->blockSize);
        }
        PLY_INLINE Ref(Reference<Footer>&& block, char* byte)
            : block{std::move(block)}, byte{byte} {
            PLY_ASSERT(uptr(byte - this->block->bytes) <= this->block->blockSize);
        }
        PLY_INLINE Ref(const WeakRef& weakRef) : block{weakRef.block}, byte{weakRef.byte} {
        }
        PLY_INLINE operator WeakRef() const {
            return {this->block, this->byte};
        }
        PLY_INLINE void operator=(const WeakRef& weakRef) {
            this->block = weakRef.block;
            this->byte = weakRef.byte;
        }
        PLY_INLINE void operator=(Ref&& other) {
            this->block = std::move(other.block);
            this->byte = other.byte;
        }
    };

    //--------------------------------------
    // RangeForIterator
    //--------------------------------------
    struct RangeForIterator {
        WeakRef curPos;
        WeakRef endPos;

        RangeForIterator(const WeakRef& startPos, const WeakRef& endPos = {})
            : curPos{startPos}, endPos{endPos} {
        }
        PLY_INLINE RangeForIterator& begin() {
            return *this;
        }
        PLY_INLINE RangeForIterator& end() {
            return *this;
        }
        PLY_INLINE void operator++() {
            this->curPos = this->curPos.block->weakRefToNext();
        }
        PLY_INLINE bool operator!=(const RangeForIterator&) const {
            return this->curPos != this->endPos;
        }
        PLY_INLINE StringView operator*() const {
            return StringView::fromRange(this->curPos.byte,
                                         (this->curPos.block == this->endPos.block)
                                             ? this->endPos.byte
                                             : this->curPos.block->unused());
        }
    };

    //--------------------------------------
    // Static member functions
    //--------------------------------------
    static PLY_DLL_ENTRY Reference<Footer> createBlock(u32 numBytes = DefaultBlockSize);
    static PLY_DLL_ENTRY Reference<Footer> createOverlayBlock(const WeakRef& pos, u32 numBytes);
    static PLY_DLL_ENTRY Footer* appendBlock(Footer* block, u32 numBytes = DefaultBlockSize);
    static PLY_DLL_ENTRY void appendBlockWithRecycle(Reference<Footer>& block,
                                                     u32 numBytes = DefaultBlockSize);
    static PLY_INLINE RangeForIterator iterateOverViews(const WeakRef& start,
                                                        const WeakRef& end = {}) {
        return {start, end};
    }
    static PLY_DLL_ENTRY u32 jumpToNextBlock(WeakRef* weakRef);
    static PLY_DLL_ENTRY u32 jumpToPrevBlock(WeakRef* weakRef);

    /*!
    Returns a `String` that owns a newly allocated block of memory containing a copy of the data
    contained in some (possible multiple) blocks between the `start` and `end` cursors. If `end`
    is not specified, the returned string contains a copy of the data from `start` up to the end
    of the block list.

    The first argument will be reset to an empty `Ref` as a result of this call. This enables an
    optimization: If `start` is the sole reference to the start of a single `Footer`, no
    data is copied; instead, the block's memory block is truncated and ownership is passed
    directly to the `String`.

    This function is used internally by `StringOutStream::moveToString()`.
    */
    static PLY_DLL_ENTRY String toString(Ref&& start, const WeakRef& end = {});

    //--------------------------------------
    // BlockList object
    //--------------------------------------
    Reference<Footer> head;
    Footer* tail = nullptr;

    PLY_DLL_ENTRY BlockList();
    PLY_DLL_ENTRY ~BlockList();
    PLY_DLL_ENTRY char* appendBytes(u32 numBytes);
    PLY_DLL_ENTRY void popLastBytes(u32 numBytes);
    PLY_INLINE WeakRef end() const {
        return {tail, tail->unused()};
    }
};

} // namespace ply

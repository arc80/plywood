/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/container/BlockList.h>

namespace ply {
namespace details {

struct SequenceImpl {
    Reference<BlockList::Footer> head;
    BlockList::Footer* tail;

    SequenceImpl();
    SequenceImpl(SequenceImpl&& other);
    ~SequenceImpl();
    void operator=(SequenceImpl&& other);
    char* beginWriteInternal(u32 numRequestedBytes);
    String moveToString();

    PLY_INLINE bool isEmpty() {
        return (this->head == this->tail) && (this->head->numBytesUsed == 0);
    }
    PLY_INLINE void* last(u32 itemSize) {
        PLY_ASSERT(this->tail->numBytesUsed >= itemSize);
        return this->tail->unused() - itemSize;
    }
};

void destructSequenceInternal(BlockList::Footer* block, void (*dtor)(void*), u32 itemSize);

template <typename T, std::enable_if_t<std::is_trivially_destructible<T>::value, int> = 0>
PLY_INLINE void destructSequence(BlockList::Footer*) {
    // Trivially destructible
}

template <typename T, std::enable_if_t<!std::is_trivially_destructible<T>::value, int> = 0>
PLY_INLINE void destructSequence(BlockList::Footer* block) {
    // Explicitly destructble
    auto dtor = [](void* item) { //
        reinterpret_cast<T*>(item)->~T();
    };
    destructSequenceInternal(block, dtor, safeDemote<u32>(sizeof(T)));
}

struct SequenceReaderImpl {
    char *curByte = nullptr;
    char* endByte = nullptr;
    Reference<BlockList::Footer> block;

    PLY_INLINE SequenceReaderImpl() = default;
    SequenceReaderImpl(const SequenceImpl& tb);
    PLY_INLINE bool atEOF() const {
        return (this->curByte >= this->endByte) && !this->block->nextBlock;
    }
    PLY_INLINE u32 numBytesAvailable() const {
        return safeDemote<u32>(endByte - curByte);
    }

    u32 tryMakeBytesAvailableInternal(u32 numRequestedBytes);
    PLY_INLINE u32 tryMakeBytesAvailable(u32 numRequestedBytes) {
        u32 available = this->numBytesAvailable();
        if (numRequestedBytes > available) {
            return tryMakeBytesAvailableInternal(numRequestedBytes);
        } else {
            return available;
        }
    }
};

} // namespace details
} // namespace ply

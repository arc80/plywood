/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/container/ChunkList.h>

namespace ply {
namespace details {

struct TypedListImpl {
    Reference<ChunkListNode> tailChunk;
    Reference<ChunkListNode> headChunk;

    TypedListImpl();
    TypedListImpl(TypedListImpl&& other);
    ~TypedListImpl();
    void operator=(TypedListImpl&& other);
    char* beginWriteInternal(u32 numRequestedBytes);
    String moveToString();

    PLY_INLINE bool isEmpty() {
        return (this->tailChunk->writePos == 0) && (this->headChunk == this->tailChunk);
    }
};

void destructTypedListInternal(ChunkListNode* chunk, void (*dtor)(void*), u32 itemSize);

template <typename T, std::enable_if_t<std::is_trivially_destructible<T>::value, int> = 0>
PLY_INLINE void destructTypedList(ChunkListNode*) {
    // Trivially destructible
}

template <typename T, std::enable_if_t<!std::is_trivially_destructible<T>::value, int> = 0>
PLY_INLINE void destructTypedList(ChunkListNode* chunk) {
    // Explicitly destructble
    auto dtor = [](void* item) { //
        reinterpret_cast<T*>(item)->~T();
    };
    destructTypedListInternal(chunk, dtor, safeDemote<u32>(sizeof(T)));
}

struct TypedListReaderImpl {
    char *curByte = nullptr;
    char* endByte = nullptr;
    Reference<ChunkListNode> chunk;

    PLY_INLINE TypedListReaderImpl() = default;
    TypedListReaderImpl(const TypedListImpl& tb);
    PLY_INLINE bool atEOF() const {
        return (this->curByte >= this->endByte) && !this->chunk->next;
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

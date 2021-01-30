/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/string/String.h>
#include <ply-runtime/memory/Heap.h>

namespace ply {

#if PLY_WITH_ASSERTS
#define PLY_WITH_BINARY_BUFFER_CHECKS PLY_DEBUG_BUILD
#endif

class BinaryBufferReader;

//-----------------------------------------------------------------
// Obsolete class
// FIXME: Delete and replace with ChunkList everywhere
//-----------------------------------------------------------------
class BinaryBuffer {
public:
    struct ChunkListNode {
        static const u32 Alignment = 16;

        ChunkListNode* next;
        u32 writePos;
        u32 allocated;
#if PLY_WITH_BINARY_BUFFER_CHECKS
        mutable s32 numReaders;
#endif
#if PLY_COMPILER_MSVC
#pragma warning(push)
#pragma warning(disable : 4200) // nonstandard extension used: zero-sized array in struct/union
#endif
        u8 data[];
#if PLY_COMPILER_MSVC
#pragma warning(pop)
#endif

        static ChunkListNode* create(u32 size, u32 alignment = 0) {
            alignment = max(Alignment, alignment);
            u32 allocSize = (u32)(alignPowerOf2(sizeof(ChunkListNode), alignment) +
                                  alignPowerOf2(size, alignment));
            ChunkListNode* chunk = (ChunkListNode*) PLY_HEAP.allocAligned(allocSize, alignment);
            chunk->next = nullptr;
            chunk->writePos = 0;
            chunk->allocated = size;
#if PLY_WITH_BINARY_BUFFER_CHECKS
            chunk->numReaders = 0;
#endif
            return chunk;
        }

        void destroy() {
#if PLY_WITH_BINARY_BUFFER_CHECKS
            PLY_ASSERT(numReaders == 0);
#endif
            PLY_HEAP.freeAligned(this);
        }
    };

    static const u32 DefaultChunkSize = 1024;

    ChunkListNode* m_first = nullptr;
    ChunkListNode* m_last = nullptr;
    u32 m_headPos = 0;
    u32 m_chunkSize;
    u32 m_sizeOfCompleteChunks = 0;
#if PLY_WITH_BINARY_BUFFER_CHECKS
    bool m_isWriting = false;
#endif

    BinaryBuffer(u32 chunkSize = DefaultChunkSize) : m_chunkSize(chunkSize) {
    }

    BinaryBuffer(BinaryBuffer&& other) {
#if PLY_WITH_BINARY_BUFFER_CHECKS
        PLY_ASSERT(!other.m_isWriting);
#endif
        m_first = other.m_first;
        m_last = other.m_last;
        m_headPos = other.m_headPos;
        m_chunkSize = other.m_chunkSize;
        m_sizeOfCompleteChunks = other.m_sizeOfCompleteChunks;
        other.m_first = nullptr;
        other.m_last = nullptr;
        other.m_headPos = 0;
        other.m_sizeOfCompleteChunks = 0;
    }

    ~BinaryBuffer() {
        clear();
    }

    void operator=(BinaryBuffer&& other) {
#if PLY_WITH_BINARY_BUFFER_CHECKS
        PLY_ASSERT(!other.m_isWriting);
#endif
        m_first = other.m_first;
        m_last = other.m_last;
        m_headPos = other.m_headPos;
        m_chunkSize = other.m_chunkSize;
        m_sizeOfCompleteChunks = other.m_sizeOfCompleteChunks;
        other.m_first = nullptr;
        other.m_last = nullptr;
        other.m_headPos = 0;
        other.m_sizeOfCompleteChunks = 0;
    }

    bool isEmpty() const {
        PLY_ASSERT(m_first == nullptr || m_first->writePos > 0);
        return m_first == nullptr;
    }

    u32 numBytes() const {
        return m_sizeOfCompleteChunks + (m_last ? m_last->writePos : 0);
    }

    void clear() {
#if PLY_WITH_BINARY_BUFFER_CHECKS
        PLY_ASSERT(!m_isWriting);
#endif
        for (ChunkListNode* chunk = m_first; chunk;) {
            ChunkListNode* next = chunk->next;
            chunk->destroy();
            chunk = next;
        }
        m_first = nullptr;
        m_last = nullptr;
        m_headPos = 0;
        m_sizeOfCompleteChunks = 0;
    }

    void* beginWrite(u32* size) {
#if PLY_WITH_BINARY_BUFFER_CHECKS
        PLY_ASSERT(!m_isWriting);
        m_isWriting = true;
#endif
        if (!m_last) {
            PLY_ASSERT(!m_first);
            m_first = ChunkListNode::create(max(*size, m_chunkSize));
            m_last = m_first;
        } else if (m_last->writePos >= m_last->allocated ||
                   m_last->writePos + *size > m_last->allocated) {
            ChunkListNode* next = ChunkListNode::create(max(*size, m_chunkSize));
            m_last->next = next;
            m_sizeOfCompleteChunks += m_last->writePos;
            m_last = next;
        }
        PLY_ASSERT(m_last->writePos + *size <= m_last->allocated);
        *size = m_last->allocated - m_last->writePos;
        return PLY_PTR_OFFSET(m_last->data, m_last->writePos);
    }

    void* beginWrite(u32 size) {
        return beginWrite(&size);
    }

    void endWrite(u32 size) {
#if PLY_WITH_BINARY_BUFFER_CHECKS
        PLY_ASSERT(m_isWriting);
        m_isWriting = false;
#endif
        PLY_ASSERT(m_last);
        PLY_ASSERT(m_last->writePos + size <= m_last->allocated);
        m_last->writePos += size;
    }

    void write(StringView data) {
        void* dst = beginWrite(safeDemote<u32>(data.numBytes));
        memcpy(dst, data.bytes, data.numBytes);
        endWrite(data.numBytes);
    }

    template <typename T>
    void write(const T& item) {
        // Note: There's nothing here to take care of alignment
        PLY_STATIC_ASSERT(std::is_trivially_destructible<T>::value);
        void* dst = beginWrite(sizeof(T));
        new (dst) T{item};
        endWrite(sizeof(T));
    }

    String flatCopy() const {
        String result;
        result.resize(numBytes());
        char* current = result.bytes;
        char* end = current + result.numBytes;
        PLY_UNUSED(end);
        for (ChunkListNode* chunk = m_first; chunk;) {
            PLY_ASSERT(current + chunk->writePos <= end);
            memcpy(current, chunk->data, chunk->writePos);
            current += chunk->writePos;
            chunk = chunk->next;
        }
        PLY_ASSERT(current == end);
        return result;
    }

    void* tryBeginPop(u32& size) {
        if (!m_first) {
            size = 0;
            return nullptr;
        }
#if PLY_WITH_BINARY_BUFFER_CHECKS
        PLY_ASSERT(m_first->numReaders == 0);
#endif
        PLY_ASSERT(m_headPos < m_first->writePos);
        size = m_first->writePos - m_headPos;
        return PLY_PTR_OFFSET(m_first->data, m_headPos);
    }

    void* beginPop(u32& size) {
        PLY_ASSERT(m_first);
#if PLY_WITH_BINARY_BUFFER_CHECKS
        PLY_ASSERT(m_first->numReaders == 0);
#endif
        PLY_ASSERT(m_headPos < m_first->writePos);
        size = m_first->writePos - m_headPos;
        return PLY_PTR_OFFSET(m_first->data, m_headPos);
    }

    void endPop(u32 size) {
        PLY_ASSERT(m_first);
#if PLY_WITH_BINARY_BUFFER_CHECKS
        PLY_ASSERT(m_first->numReaders == 0);
#endif
        PLY_ASSERT(m_headPos + size <= m_first->writePos);
        m_headPos += size;
        if (m_headPos >= m_first->writePos) {
            ChunkListNode* chunk = m_first;
            m_first = chunk->next;
            chunk->destroy();
            m_headPos = 0;
        }
        PLY_ASSERT(!m_first || m_headPos < m_first->writePos);
    }

    void pop(u32 size) {
        while (size > 0) {
            u32 sizeAvail;
            beginPop(sizeAvail);
            sizeAvail = min(sizeAvail, size);
            endPop(sizeAvail);
            size -= sizeAvail;
        }
    }

    /*
    template <typename T>
    T* append() {
        T* item = (T*) beginWrite(sizeof(T));
        new(item) T;
        endWrite(sizeof(T));
        return item;
    }

    template <typename T>
    void write(T&& item) {
        T* dstItem = (T*) beginWrite(sizeof(T));
        new(dstItem) T{std::move(item)};
        endWrite(sizeof(T));
    }

    template <typename T, typename... Args>
    T* emplace(Args&&... args) {
        T* item = (T*) beginWrite(sizeof(T));
        new(item) T(std::forward<Args>(args)...);
        endWrite(sizeof(T));
        return item;
    }
    */

    BinaryBufferReader getCursorAt(const void* pos);
};

//-----------------------------------------------------------------
// BinaryBufferReader
//-----------------------------------------------------------------
class BinaryBufferReader {
public:
    BinaryBuffer::ChunkListNode* m_chunk;
    u32 m_readPos;

    BinaryBufferReader(const BinaryBuffer& src) {
        m_chunk = src.m_first;
        m_readPos = src.m_headPos;
#if PLY_WITH_BINARY_BUFFER_CHECKS
        if (m_chunk) {
            m_chunk->numReaders++;
        }
#endif
    }

    BinaryBufferReader(const BinaryBufferReader& other) {
        m_chunk = other.m_chunk;
        m_readPos = other.m_readPos;
#if PLY_WITH_BINARY_BUFFER_CHECKS
        if (m_chunk) {
            m_chunk->numReaders++;
        }
#endif
    }

    BinaryBufferReader(BinaryBuffer::ChunkListNode* chunk, u32 readPos)
        : m_chunk{chunk}, m_readPos{readPos} {
#if PLY_WITH_BINARY_BUFFER_CHECKS
        if (m_chunk) {
            m_chunk->numReaders++;
        }
#endif
    }

    ~BinaryBufferReader() {
#if PLY_WITH_BINARY_BUFFER_CHECKS
        if (m_chunk) {
            m_chunk->numReaders--;
            PLY_ASSERT(m_chunk->numReaders >= 0);
        }
#endif
    }

    void operator=(const BinaryBufferReader& other) {
#if PLY_WITH_BINARY_BUFFER_CHECKS
        if (m_chunk) {
            m_chunk->numReaders--;
            PLY_ASSERT(m_chunk->numReaders >= 0);
        }
#endif
        m_chunk = other.m_chunk;
        m_readPos = other.m_readPos;
#if PLY_WITH_BINARY_BUFFER_CHECKS
        if (m_chunk) {
            m_chunk->numReaders++;
        }
#endif
    }

    void* tryBeginRead(u32& size) const {
        if (!m_chunk) {
            size = 0;
            return nullptr;
        }
#if PLY_WITH_BINARY_BUFFER_CHECKS
        PLY_ASSERT(m_chunk->numReaders > 0);
#endif
        PLY_ASSERT(m_readPos < m_chunk->writePos);
        size = m_chunk->writePos - m_readPos;
        return PLY_PTR_OFFSET(m_chunk->data, m_readPos);
    }

    void* beginRead(u32& size) const {
        PLY_ASSERT(m_chunk);
#if PLY_WITH_BINARY_BUFFER_CHECKS
        PLY_ASSERT(m_chunk->numReaders > 0);
#endif
        PLY_ASSERT(m_readPos < m_chunk->writePos);
        size = m_chunk->writePos - m_readPos;
        return PLY_PTR_OFFSET(m_chunk->data, m_readPos);
    }

    void endRead(u32 size) {
        PLY_ASSERT(m_chunk);
        PLY_ASSERT(m_readPos + size <= m_chunk->writePos);
        m_readPos += size;
        if (m_readPos >= m_chunk->writePos) {
#if PLY_WITH_BINARY_BUFFER_CHECKS
            m_chunk->numReaders--;
            PLY_ASSERT(m_chunk->numReaders >= 0);
#endif
            m_chunk = m_chunk->next;
#if PLY_WITH_BINARY_BUFFER_CHECKS
            if (m_chunk) {
                m_chunk->numReaders++;
            }
#endif
            m_readPos = 0;
        }
        PLY_ASSERT(!m_chunk || m_readPos < m_chunk->writePos);
    }

    bool operator!=(const BinaryBufferReader& other) const {
        return m_chunk != other.m_chunk || m_readPos != other.m_readPos;
    }
};

inline BinaryBufferReader BinaryBuffer::getCursorAt(const void* pos) {
    PLY_ASSERT(pos > m_last->data && pos <= m_last->data + m_last->writePos);
    return {m_last, (u32)((u8*) pos - m_last->data)};
}

inline bool match(BinaryBufferReader& reader0, BinaryBufferReader& reader1) {
    for (;;) {
        u32 size0;
        u32 size1;
        void* ptr0 = reader0.tryBeginRead(size0);
        void* ptr1 = reader1.tryBeginRead(size1);
        PLY_ASSERT((size0 == 0) == (size1 == 0));
        u32 sizeToCompare = min(size0, size1);
        if (sizeToCompare == 0)
            return true; // They match
        if (memcmp(ptr0, ptr1, sizeToCompare) != 0)
            return false; // They don't match
        reader0.endRead(sizeToCompare);
        reader1.endRead(sizeToCompare);
    }
}

} // namespace ply

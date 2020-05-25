/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/container/BinaryBuffer.h>
#include <ply-runtime/container/Array.h>

namespace ply {

template <typename T>
class TypedBufferReader;

template <typename T>
class TypedBufferIterator;

//-----------------------------------------------------------
// TypedBuffer
//-----------------------------------------------------------
template <typename T>
class TypedBuffer {
private:
    static const u32 DefaultItemsPerChunk = sizeof(T) < 32 ? 1024 / sizeof(T) : 32;

    BinaryBuffer m_buffer;
    friend class TypedBufferReader<T>;

public:
    TypedBuffer(u32 itemsPerChunk = DefaultItemsPerChunk)
        : m_buffer{(u32) sizeof(T) * itemsPerChunk} {
    }

    TypedBuffer(TypedBuffer&& other) : m_buffer{std::move(other.m_buffer)} {
    }

    ~TypedBuffer() {
        clear();
    }

    void operator=(TypedBuffer&& other) {
        m_buffer = std::move(other.m_buffer);
    }

    bool isEmpty() const {
        return m_buffer.isEmpty();
    }

    u32 numItems() const {
        // FIXME: If we make a BaseBuffer that doesn't know its own itemSize, we could make it
        // store an abstract number of items and avoid the divide here:
        return m_buffer.numBytes() / sizeof(T);
    }

    T* beginWrite() {
        return (T*) m_buffer.beginWrite(sizeof(T));
    }

    void endWrite() {
        m_buffer.endWrite(sizeof(T));
    }

    T* write(const T& item) {
        T* result = beginWrite();
        new (result) T{item};
        endWrite();
        return result;
    }

    T* write(T&& item) {
        T* result = beginWrite();
        new (result) T{std::move(item)};
        endWrite();
        return result;
    }

    template <typename... Args>
    T* append(Args&&... args) {
        T* result = beginWrite();
        new (result) T{std::forward<Args>(args)...};
        endWrite();
        return result;
    }

    T* tryBeginPop() {
        u32 size;
        void* ptr = m_buffer.tryBeginPop(size);
        PLY_ASSERT(!ptr || size >= sizeof(T));
        return (T*) ptr;
    }

    T* beginPop() {
        u32 size;
        void* ptr = m_buffer.beginPop(size);
        PLY_ASSERT(size >= sizeof(T));
        return (T*) ptr;
    }

    void endPop() {
        u32 size;
        void* ptr = m_buffer.beginPop(size); // Multiple beginPop()s is OK
        PLY_ASSERT(size >= sizeof(T));
        ((T*) ptr)->~T();
        m_buffer.endPop(sizeof(T));
    }

    void pop(u32 popCount = 1) {
        for (; popCount > 0; popCount--) {
            // It's OK to omit the beginPop()
            endPop();
        }
    }

    void clear() {
        while (tryBeginPop()) {
            endPop();
        }
        PLY_ASSERT(m_buffer.isEmpty());
    }

    class WriteGuard {
    private:
        TypedBuffer& m_buffer;
        T* m_item;
        friend class TypedBuffer;

        WriteGuard(TypedBuffer& buffer) : m_buffer{buffer} {
            m_item = m_buffer.beginWrite();
        };

    public:
        ~WriteGuard() {
            m_buffer.endWrite();
        }

        T* operator->() const {
            return m_item;
        }
    };

    WriteGuard writeGuard() {
        return {*this};
    }

    TypedBufferIterator<T> begin();
    TypedBufferIterator<T> end();
    TypedBufferIterator<const T> begin() const;
    TypedBufferIterator<const T> end() const;
};

//-----------------------------------------------------------
// TypedBufferReader
//-----------------------------------------------------------
template <typename T>
class TypedBufferReader {
private:
    BinaryBufferReader m_reader;

public:
    TypedBufferReader(const TypedBuffer<T>& src) : m_reader{src.m_buffer} {
    }

    // FIXME: Should this return an ArrayView<T>?
    T* tryBeginRead(u32& size) {
        u32 rawSize;
        void* ptr = m_reader.tryBeginRead(rawSize);
        PLY_ASSERT(!ptr || rawSize >= sizeof(T));
        size = rawSize / sizeof(T);
        return (T*) ptr;
    }

    T* beginRead(u32& size) {
        u32 rawSize;
        void* ptr = m_reader.beginRead(rawSize);
        PLY_ASSERT(rawSize >= sizeof(T));
        size = rawSize / sizeof(T);
        return (T*) ptr;
    }

    void endRead(u32 size) {
        m_reader.endRead(size * sizeof(T));
    }

    T read() {
        u32 size;
        T result = *beginRead(size);
        endRead(1);
        return result;
    }
};

//-----------------------------------------------------------
// TypedBufferIterator
//-----------------------------------------------------------
template <typename T>
class TypedBufferIterator {
private:
    BinaryBufferReader m_reader;
    friend class TypedBuffer<T>;
    friend class TypedBuffer<typename std::remove_const<T>::type>;

    TypedBufferIterator(BinaryBuffer::ChunkListNode* chunk, u32 readPos)
        : m_reader{chunk, readPos} {
    }

public:
    bool operator!=(const TypedBufferIterator& other) const {
        return m_reader != other.m_reader;
    }

    void operator++() {
        m_reader.endRead(sizeof(T));
    }

    T* operator*() const {
        u32 size;
        void* ptr = m_reader.beginRead(size);
        PLY_ASSERT(size >= sizeof(T));
        return (T*) ptr;
    }

    T* operator->() const {
        u32 size;
        void* ptr = m_reader.beginRead(size);
        PLY_ASSERT(size >= sizeof(T));
        return (T*) ptr;
    }
};

//-----------------------------------------------------------
// Member functions
//-----------------------------------------------------------
template <typename T>
TypedBufferIterator<T> TypedBuffer<T>::begin() {
    return {m_buffer.m_first, m_buffer.m_headPos};
}

template <typename T>
TypedBufferIterator<T> TypedBuffer<T>::end() {
    return {nullptr, 0};
}

template <typename T>
TypedBufferIterator<const T> TypedBuffer<T>::begin() const {
    return {m_buffer.m_first, m_buffer.m_headPos};
}

template <typename T>
TypedBufferIterator<const T> TypedBuffer<T>::end() const {
    return {nullptr, 0};
}

//-----------------------------------------------------------
// Convert to Array
//-----------------------------------------------------------
template <typename T>
Array<T> moveToArray(const TypedBuffer<T>& buffer) {
    Array<T> result;
    result.reserve(buffer.numItems());
    TypedBufferReader<T> reader{buffer};
    u32 sizeToMove;
    while (T* src = reader.tryBeginRead(sizeToMove)) {
        result.moveExtend(ArrayView<T>{src, sizeToMove});
        reader.endRead(sizeToMove);
    }
    PLY_ASSERT(result.numItems() == buffer.numItems());
    return result;
}

} // namespace ply

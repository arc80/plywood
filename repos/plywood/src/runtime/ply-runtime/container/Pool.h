/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/container/Array.h>

#if PLY_WITH_ASSERTS
#define PLY_WITH_POOL_DEBUG_CHECKS 1
#endif

namespace ply {

template <typename T, typename Index>
class PoolIndex;
template <typename T>
class PoolIterator;
template <typename T>
class PoolPtr;

//-----------------------------------------------------------------------
// BasePool
//-----------------------------------------------------------------------
class BasePool {
public:
    void* m_items = nullptr;
    u32 m_size = 0;
    u32 m_allocated = 0;
    mutable u32 m_firstFree = u32(-1);
    mutable u32 m_sortedFreeList : 1;
    u32 m_freeListSize : 31;
#if PLY_WITH_POOL_DEBUG_CHECKS
    mutable u16 m_numReaders = 0;
#endif

    BasePool();
    void baseReserve(u32 newSize, u32 itemSize);
    u32 baseAlloc(u32 itemSize);
    void baseFree(u32 index, u32 itemSize);
    PLY_DLL_ENTRY void baseSortFreeList(u32 stride) const;
    void baseClear();
};

//-----------------------------------------------------------------------
// Pool<T>
//-----------------------------------------------------------------------
template <typename T_>
class Pool : public BasePool {
public:
    typedef T_ T;
    PLY_STATIC_ASSERT(sizeof(T) >= sizeof(u32));

    Pool();
    void clear();
    ~Pool();
    template <typename Index, typename... Args>
    PoolIndex<T, Index> newItem(Args&&... args);
    template <typename Index>
    void delItem(PoolIndex<T, Index> index);

    template <typename Index>
    PoolPtr<T> get(PoolIndex<T, Index>);
    template <typename Index>
    PoolPtr<const T> get(PoolIndex<T, Index>) const;
    u32 indexOf(const T* item) const;

    PoolIterator<T> begin();
    PoolIterator<T> end();
    PoolIterator<const T> begin() const;
    PoolIterator<const T> end() const;
};

//-----------------------------------------------------------------------
// PoolIndex<T, Index>
//-----------------------------------------------------------------------
template <typename T_, typename Index_>
class PoolIndex {
public:
    typedef T_ T;
    typedef Index_ Index;
    PLY_STATIC_ASSERT((Index) -1 > 0); // Should be an unsigned type
    static constexpr Index InvalidIndex = (Index) -1;

    Index idx = InvalidIndex;

    PoolIndex() = default;
    PoolIndex(u32 idx);
    PoolIndex(const PoolIndex<const T, Index>& other);
    // Don't allow implicit cast to integer type (and no implicit bool cast either!)
    bool isValid() const;
    bool operator==(const PoolIndex& other) const;
    bool operator!=(const PoolIndex& other) const;
};

//-----------------------------------------------------------------------
// OwnPoolHandle<T, Index>
//-----------------------------------------------------------------------
template <typename Traits>
class OwnPoolHandle : public PoolIndex<typename Traits::T, typename Traits::Index> {
public:
    OwnPoolHandle() = default;
    OwnPoolHandle(u32 idx);
    OwnPoolHandle(OwnPoolHandle&& other);
    ~OwnPoolHandle();

    void assign(PoolIndex<typename Traits::T, typename Traits::Index> other);
    void clear();
    PoolPtr<typename Traits::T> get() const;
};

//---------------------------------------------------------
// PoolIterator
//---------------------------------------------------------
template <typename T>
class PoolIterator {
private:
    const Pool<T>* m_pool;
    u32 m_nextFree;
    u32 m_index;

public:
    PoolIterator(const Pool<T>* pool, u32 nextFree, u32 index);
    ~PoolIterator();
    PoolIterator(const PoolIterator& other);
    void operator=(const PoolIterator&) = delete;
    template <typename Index>
    PoolIndex<T, Index> getIndex() const;
    PoolPtr<T> operator*() const;
    PoolIterator& operator++();
    bool isValid() const;
    bool operator==(const PoolIterator& other) const;
    bool operator!=(const PoolIterator& other) const;
};

//---------------------------------------------------------
// PoolPtr
//---------------------------------------------------------
template <typename T>
class PoolPtr {
public:
#if PLY_WITH_POOL_DEBUG_CHECKS
    const Pool<T>* pool;
#endif
    T* ptr;

    PoolPtr();
    PoolPtr(const Pool<T>* pool, T* ptr);
    PoolPtr(const PoolPtr<T>& other);
    ~PoolPtr();
    void operator=(const PoolPtr<T>& other);
    bool operator==(const PoolPtr<T>& other) const;
    operator T*() const = delete; // Disallow cast to pointer because we don't want callers to
                                  // accidentally store the pointer
    T* get() const;
    explicit operator bool() const;
    T* operator->() const;
    T* release();
    // Allow implicit conversion to PoolPtr<const T>&
    operator const PoolPtr<const T>&() const;
};

//------------------------------------------------------------------------------------
// PoolIndex<T, Index> inline functions
//------------------------------------------------------------------------------------
template <typename T, typename Index>
PoolIndex<T, Index>::PoolIndex(u32 idx) : idx{safeDemote<Index>(idx)} {
}

template <typename T, typename Index>
PoolIndex<T, Index>::PoolIndex(const PoolIndex<const T, Index>& other) : idx{other.idx} {
}

template <typename T, typename Index>
bool PoolIndex<T, Index>::isValid() const {
    return idx != InvalidIndex;
}

template <typename T, typename Index>
bool PoolIndex<T, Index>::operator==(const PoolIndex& other) const {
    return idx == other.idx;
}

template <typename T, typename Index>
bool PoolIndex<T, Index>::operator!=(const PoolIndex& other) const {
    return idx != other.idx;
}

//------------------------------------------------------------------------------------
// PoolPtr<T> inline functions
//------------------------------------------------------------------------------------
template <typename T>
PoolPtr<T>::PoolPtr()
    :
#if PLY_WITH_POOL_DEBUG_CHECKS
      pool{nullptr},
#endif
      ptr{nullptr} {
}

template <typename T>
PoolPtr<T>::PoolPtr(const Pool<T>* pool, T* ptr)
    :
#if PLY_WITH_POOL_DEBUG_CHECKS
      pool{pool},
#endif
      ptr{ptr} {
#if PLY_WITH_POOL_DEBUG_CHECKS
    pool->m_numReaders++;
#endif
}

template <typename T>
PoolPtr<T>::PoolPtr(const PoolPtr<T>& other)
    :
#if PLY_WITH_POOL_DEBUG_CHECKS
      pool{(const Pool<T>*) other.pool},
#endif
      ptr{other.ptr} {
#if PLY_WITH_POOL_DEBUG_CHECKS
    if (pool) {
        pool->m_numReaders++;
    }
#endif
}

template <typename T>
PoolPtr<T>::~PoolPtr() {
#if PLY_WITH_POOL_DEBUG_CHECKS
    PLY_ASSERT((bool) pool == (bool) ptr); // Either both nullptr, or neither nullptr
    if (pool) {
        PLY_ASSERT(pool->m_numReaders > 0);
        pool->m_numReaders--;
    }
#endif
}

template <typename T>
bool PoolPtr<T>::operator==(const PoolPtr<T>& other) const {
    PLY_ASSERT(ptr != other.ptr || pool == other.pool);
    return ptr == other.ptr;
}

template <typename T>
void PoolPtr<T>::operator=(const PoolPtr<T>& other) {
#if PLY_WITH_POOL_DEBUG_CHECKS
    if (pool) {
        pool->m_numReaders--;
    }
    pool = (const Pool<T>*) other.pool;
    if (pool) {
        pool->m_numReaders++;
    }
#endif
    ptr = other.ptr;
}

template <typename T>
T* PoolPtr<T>::get() const {
    return ptr;
}

template <typename T>
PoolPtr<T>::operator bool() const {
    return ptr != nullptr;
}

template <typename T>
T* PoolPtr<T>::operator->() const {
    return ptr;
}

template <typename T>
T* PoolPtr<T>::release() {
#if PLY_WITH_POOL_DEBUG_CHECKS
    PLY_ASSERT((bool) pool == (bool) ptr); // Either both nullptr, or neither nullptr
    if (pool) {
        PLY_ASSERT(pool->m_numReaders > 0);
        pool->m_numReaders--;
    }
    pool = nullptr;
#endif
    T* r = ptr;
    ptr = nullptr;
    return r;
}

// Allow implicit conversion to PoolPtr<const T>&
template <typename T>
PoolPtr<T>::operator const PoolPtr<const T>&() const {
    return *(const PoolPtr<const T>*) this;
}

//------------------------------------------------------------------------------------
// BasePool inline functions
//------------------------------------------------------------------------------------
inline BasePool::BasePool() {
    m_sortedFreeList = 1;
    m_freeListSize = 0;
}

inline void BasePool::baseReserve(u32 newSize, u32 itemSize) {
    m_allocated = (u32) roundUpPowerOf2(
        max<u32>(newSize, 8));    // FIXME: Generalize to other resize strategies when needed
    PLY_ASSERT(m_allocated != 0); // Overflow check
    m_items = PLY_HEAP.realloc(
        m_items, itemSize * m_allocated); // FIXME: Generalize to other heaps when needed
}

inline u32 BasePool::baseAlloc(u32 itemSize) {
    if (m_firstFree != u32(-1)) {
        u32 nextFree = *(u32*) PLY_PTR_OFFSET(m_items, m_firstFree * itemSize);
        u32 index = m_firstFree;
        m_firstFree = nextFree;
        m_freeListSize--;
        PLY_ASSERT((m_freeListSize == 0) == (m_firstFree == u32(-1)));
        return index;
    }
    if (m_size >= m_allocated) {
        baseReserve(m_size + 1, itemSize);
    }
    return m_size++;
}

inline void BasePool::baseFree(u32 index, u32 itemSize) {
    PLY_ASSERT(index < m_size);
    *(u32*) PLY_PTR_OFFSET(m_items, index * itemSize) = m_firstFree;
    m_firstFree = index;
    m_sortedFreeList = 0;
    PLY_ASSERT(m_freeListSize < (1u << 31) - 1);
    m_freeListSize++;
}

inline void BasePool::baseClear() {
    PLY_HEAP.free(m_items);
    m_items = nullptr;
    m_size = 0;
    m_allocated = 0;
    m_firstFree = -1;
    m_sortedFreeList = 1;
    m_freeListSize = 0;
}

//------------------------------------------------------------------------------------
// Pool<T> inline functions
//------------------------------------------------------------------------------------
template <typename T>
Pool<T>::Pool() {
}

template <typename T>
void Pool<T>::clear() {
    if (!std::is_trivially_destructible<T>::value) {
        baseSortFreeList(sizeof(T));
        T* item = static_cast<T*>(m_items);
        T* end = item + m_size;
        u32 skipIndex = m_firstFree;
        for (;;) {
            T* skip = static_cast<T*>(m_items) + (skipIndex == u32(-1) ? m_size : skipIndex);
            while (item < skip) {
                item->~T();
                item++;
            }
            if (item == end)
                break;
            skipIndex = *reinterpret_cast<u32*>(skip);
            item++;
        }
    }
    baseClear();
}

template <typename T>
Pool<T>::~Pool() {
    clear();
}

template <typename T>
template <typename Index, typename... Args>
PoolIndex<T, Index> Pool<T>::newItem(Args&&... args) {
#if PLY_WITH_POOL_DEBUG_CHECKS
    // There must not be any Iterators or Ptrs when modifying the pool!
    PLY_ASSERT(m_numReaders == 0);
#endif
    u32 index = baseAlloc(sizeof(T));
    new (static_cast<T*>(m_items) + index) T{std::forward<Args>(args)...};
    return index;
}

template <typename T>
template <typename Index>
void Pool<T>::delItem(PoolIndex<T, Index> index) {
#if PLY_WITH_POOL_DEBUG_CHECKS
    // There must not be any Iterators or Ptrs when modifying the pool!
    PLY_ASSERT(m_numReaders == 0);
#endif
    if (!std::is_trivially_destructible<T>::value) {
        static_cast<T*>(m_items)[index.idx].~T();
    }
    baseFree(index.idx, sizeof(T));
}

template <typename T>
template <typename Index>
PoolPtr<T> Pool<T>::get(PoolIndex<T, Index> index) {
    PLY_ASSERT(index.idx < m_size);
    return {this, static_cast<T*>(m_items) + index.idx};
}

template <typename T>
template <typename Index>
PoolPtr<const T> Pool<T>::get(PoolIndex<T, Index> index) const {
    PLY_ASSERT(index.idx < m_size);
    return {(Pool<const T>*) this, static_cast<const T*>(m_items) + index.idx};
}

template <typename T>
u32 Pool<T>::indexOf(const T* item) const {
    u32 index = safeDemote<u32>(item - static_cast<const T*>(m_items));
    PLY_ASSERT(index < m_size);
    return index;
}

//! Return iterator suitable for range-for.
//! The pool must not be modified during iteration.
template <typename T>
PoolIterator<T> Pool<T>::begin() {
    baseSortFreeList(sizeof(T));
    PoolIterator<T> iter{this, m_firstFree, u32(-1)};
    ++iter;
    return iter;
}

//! Return iterator suitable for range-for.
//! The pool must not be modified during iteration.
template <typename T>
PoolIterator<T> Pool<T>::end() {
    return {this, u32(-1), m_size};
}

//! Return iterator suitable for range-for.
//! The pool must not be modified during iteration.
template <typename T>
PoolIterator<const T> Pool<T>::begin() const {
    baseSortFreeList(sizeof(T));
    PoolIterator<const T> iter{(Pool<const T>*) this, m_firstFree, u32(-1)};
    ++iter;
    return iter;
}

//! Return iterator suitable for range-for.
//! The pool must not be modified during iteration.
template <typename T>
PoolIterator<const T> Pool<T>::end() const {
    return {this, u32(-1), m_size};
}

//------------------------------------------------------------------------------------
// PoolIterator<T> inline functions
//------------------------------------------------------------------------------------
template <typename T>
PoolIterator<T>::PoolIterator(const Pool<T>* pool, u32 nextFree, u32 index)
    : m_pool(pool), m_nextFree(nextFree), m_index(index) {
#if PLY_WITH_POOL_DEBUG_CHECKS
    m_pool->m_numReaders++;
#endif
}

template <typename T>
PoolIterator<T>::~PoolIterator() {
#if PLY_WITH_POOL_DEBUG_CHECKS
    PLY_ASSERT(m_pool->m_numReaders > 0);
    m_pool->m_numReaders--;
#endif
}

template <typename T>
PoolIterator<T>::PoolIterator(const PoolIterator& other) : m_pool(other.m_pool) {
#if PLY_WITH_POOL_DEBUG_CHECKS
    m_pool->m_numReaders++;
#endif
    m_nextFree = other.m_nextFree;
    m_index = other.m_index;
}

template <typename T>
template <typename Index>
PoolIndex<T, Index> PoolIterator<T>::getIndex() const {
    return {m_index};
}

template <typename T>
PoolPtr<T> PoolIterator<T>::operator*() const {
    PLY_ASSERT(m_index < m_pool->m_size);
    return {m_pool, static_cast<T*>(m_pool->m_items) + m_index};
}

template <typename T>
PoolIterator<T>& PoolIterator<T>::operator++() {
    m_index++;
    while (m_index < m_pool->m_size) {
        PLY_ASSERT(m_index <= m_nextFree);
        if (m_index < m_nextFree)
            break;
        m_nextFree = *reinterpret_cast<const u32*>(static_cast<T*>(m_pool->m_items) + m_index);
        m_index++;
    }
    return *this;
}

template <typename T>
bool PoolIterator<T>::isValid() const {
    return m_index < m_pool->m_size;
}

template <typename T>
bool PoolIterator<T>::operator==(const PoolIterator<T>& other) const {
    PLY_ASSERT(m_pool == other.m_pool);
    return m_index == other.m_index;
}

template <typename T>
bool PoolIterator<T>::operator!=(const PoolIterator<T>& other) const {
    PLY_ASSERT(m_pool == other.m_pool);
    return m_index != other.m_index;
}

//------------------------------------------------------------------------------------
// OwnPoolHandle<T, Index> inline functions
//------------------------------------------------------------------------------------
template <typename Traits>
OwnPoolHandle<Traits>::OwnPoolHandle(u32 idx)
    : PoolIndex<typename Traits::T, typename Traits::Index>{idx} {
}

template <typename Traits>
OwnPoolHandle<Traits>::OwnPoolHandle(OwnPoolHandle<Traits>&& other)
    : PoolIndex<typename Traits::T, typename Traits::Index>{other.idx} {
    other.idx = this->InvalidIndex;
}

template <typename Traits>
OwnPoolHandle<Traits>::~OwnPoolHandle() {
    if (this->idx != this->InvalidIndex) {
        Traits::getPool().delItem(*this);
    }
}

template <typename Traits>
void OwnPoolHandle<Traits>::assign(PoolIndex<typename Traits::T, typename Traits::Index> other) {
    if (this->idx != this->InvalidIndex) {
        Traits::getPool().delItem(*this);
    }
    this->idx = other.idx;
}

template <typename Traits>
void OwnPoolHandle<Traits>::clear() {
    if (this->idx != this->InvalidIndex) {
        Traits::getPool().delItem(*this);
    }
    this->idx = this->InvalidIndex;
}

template <typename Traits>
PoolPtr<typename Traits::T> OwnPoolHandle<Traits>::get() const {
    return Traits::getPool().get(*this);
}

} // namespace ply

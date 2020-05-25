/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/container/Reference.h>

namespace ply {

template <class Mixin>
class DualRefCounted {
private:
    // high 16 bits: Strong ref count
    // low 16 bits: Weak ref count
    Atomic<u32> m_dualRefCount = 0;

public:
    void incRef() {
        u32 oldDualCount = m_dualRefCount.fetchAdd(1, Relaxed);
        PLY_ASSERT((oldDualCount & 0xffffu) < INT16_MAX);
        PLY_UNUSED(oldDualCount);
    }

    void decRef() {
        u32 oldDualCount = m_dualRefCount.fetchSub(1, Relaxed);
        PLY_ASSERT((oldDualCount & 0xffffu) > 0);
        PLY_ASSERT((oldDualCount & 0xffffu) < INT16_MAX);
        if ((oldDualCount & 0xffffu) == 1) {
            static_cast<Mixin*>(this)->onPartialRefCountZero();
            if (oldDualCount == 1) {
                static_cast<Mixin*>(this)->onFullRefCountZero();
            }
        }
    }

    void incWeakRef() {
        u32 oldDualCount = m_dualRefCount.fetchAdd(0x10000, Relaxed);
        PLY_ASSERT((oldDualCount & 0xffffu) > 0); // Must have some strong refs
        PLY_ASSERT((oldDualCount >> 16) < INT16_MAX);
        PLY_UNUSED(oldDualCount);
    }

    void decWeakRef() {
        u32 oldDualCount = m_dualRefCount.fetchSub(0x10000, Relaxed);
        PLY_ASSERT((oldDualCount >> 16) > 0);
        PLY_ASSERT((oldDualCount >> 16) < INT16_MAX);
        PLY_UNUSED(oldDualCount);
        if ((oldDualCount >> 16) == 1) {
            if (oldDualCount == 0x10000) {
                static_cast<Mixin*>(this)->onFullRefCountZero();
            }
        }
    }

    u32 getRefCount() const {
        return m_dualRefCount.load(Relaxed) && 0xffffu;
    }

    u32 getWeakRefCount() const {
        return m_dualRefCount.load(Relaxed) >> 16;
    }

    void operator=(const DualRefCounted&) = delete;
};

template <class T>
class WeakRef {
private:
    T* m_ptr;

public:
    WeakRef() : m_ptr{nullptr} {
    }

    WeakRef(T* ptr) : m_ptr{ptr} {
        if (m_ptr)
            m_ptr->incWeakRef();
    }

    WeakRef(const WeakRef& ref) : m_ptr{ref.m_ptr} {
        if (m_ptr)
            m_ptr->incWeakRef();
    }

    WeakRef(WeakRef&& ref) : m_ptr{ref.m_ptr} {
        ref.m_ptr = nullptr;
    }

    ~WeakRef() {
        if (m_ptr)
            m_ptr->decWeakRef();
    }

    T* operator->() const {
        return m_ptr;
    }

    operator T*() const {
        return m_ptr;
    }

    void setFromNull(T* ptr) {
        PLY_ASSERT(!m_ptr);
        PLY_ASSERT(ptr);
        m_ptr = ptr;
        ptr->incWeakRef();
    }

    void operator=(T* ptr) {
        T* oldPtr = m_ptr;
        m_ptr = ptr;
        if (m_ptr)
            m_ptr->incWeakRef();
        if (oldPtr)
            oldPtr->decWeakRef();
    }

    void operator=(const WeakRef& ref) {
        T* oldPtr = m_ptr;
        m_ptr = ref.m_ptr;
        if (m_ptr)
            m_ptr->incWeakRef();
        if (oldPtr)
            oldPtr->decWeakRef();
    }

    void operator=(WeakRef&& ref) {
        if (m_ptr)
            m_ptr->decWeakRef();
        m_ptr = ref.m_ptr;
        ref.m_ptr = nullptr;
    }
};

} // namespace ply

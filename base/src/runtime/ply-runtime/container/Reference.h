/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/thread/Atomic.h>

namespace ply {

/*
Thread-safe reference counting mixin class.

Reference count is updated with atomic increment/decrement. The mixin class must define a `destroy`
function.

If manipulated from different threads, you are expected to issue a `threadFenceRelease` after the
mixin object is constructed, and the `destroy` function must perform a `threadFenceAcquire` to
ensure the initially constructed object is fully visible to `destroy`.

If not manipulated from different threads, a `RefCounted_SingleThreaded` mixin class, that modifies
the reference count non-atomically, could hypothetically be used instead.

Currently used by `assetBank::ReloadableAsset` and `graphic::ReloadableShader`. Recursive reference
loading makes thread safety important for these types, because at any time, the loading thread can
modify the reference count of an asset created by the main thread.
*/
template <typename Mixin>
class RefCounted {
private:
    Atomic<s32> m_refCount = 0;

public:
    PLY_INLINE void incRef() {
        s32 oldCount = m_refCount.fetchAdd(1, Relaxed);
        PLY_ASSERT(oldCount >= 0 && oldCount < UINT16_MAX);
        PLY_UNUSED(oldCount);
    }
    PLY_INLINE void decRef() {
        s32 oldCount = m_refCount.fetchSub(1, Relaxed);
        PLY_ASSERT(oldCount >= 1 && oldCount < UINT16_MAX);
        if (oldCount == 1) {
            static_cast<Mixin*>(this)->onRefCountZero();
        }
    }
    PLY_INLINE s32 getRefCount() const {
        return m_refCount.load(Relaxed);
    }

    // Make derived classes assignable without copying the other object's refcount:
    PLY_INLINE RefCounted() {
    }
    PLY_INLINE RefCounted(const RefCounted&) {
    }
    PLY_INLINE void operator=(const RefCounted&) {
    }
};

/*
Base class for a reference-counting handle.

You can inherit from this class to add more convenience functions to your reference type.

Currently used as a base class for `assetBank::AssetRef`.
*/
template <typename T>
class Reference {
private:
    T* ptr;

public:
    PLY_INLINE Reference() : ptr(nullptr) {
    }
    PLY_INLINE Reference(T* ptr) : ptr(ptr) {
        if (this->ptr)
            this->ptr->incRef();
    }
    PLY_INLINE Reference(const Reference& ref) : ptr(ref.ptr) {
        if (this->ptr)
            this->ptr->incRef();
    }
    PLY_INLINE Reference(Reference&& ref) : ptr(ref.ptr) {
        ref.ptr = nullptr;
    }
    PLY_INLINE ~Reference() {
        if (this->ptr)
            this->ptr->decRef();
    }
    PLY_INLINE T* operator->() const {
        return this->ptr;
    }
    PLY_INLINE operator T*() const {
        return this->ptr;
    }
    PLY_INLINE T* get() const {
        return this->ptr;
    }
    PLY_INLINE void operator=(T* ptr) {
        T* oldPtr = this->ptr;
        this->ptr = ptr;
        if (this->ptr)
            this->ptr->incRef();
        if (oldPtr)
            oldPtr->decRef();
    }

    PLY_INLINE void operator=(const Reference& ref) {
        T* oldPtr = this->ptr;
        this->ptr = ref.ptr;
        if (this->ptr)
            this->ptr->incRef();
        if (oldPtr)
            oldPtr->decRef();
    }
    PLY_INLINE void operator=(Reference&& ref) {
        if (this->ptr)
            this->ptr->decRef();
        this->ptr = ref.ptr;
        ref.ptr = nullptr;
    }
    PLY_INLINE explicit operator bool() const {
        return this->ptr != nullptr;
    }
    PLY_INLINE T* release() {
        T* ptr = this->ptr;
        this->ptr = nullptr;
        return ptr;
    };
    PLY_INLINE void clear() {
        if (this->ptr)
            this->ptr->decRef();
        this->ptr = nullptr;
    }
    PLY_INLINE bool isEmpty() const {
        return this->ptr == nullptr;
    }
};

} // namespace ply

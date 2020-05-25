/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/container/Subst.h>

namespace ply {

template <typename T>
class Borrowed {
private:
    T* ptr = nullptr;

public:
    PLY_INLINE Borrowed(T* ptr = nullptr) : ptr{ptr} {
    }
    PLY_INLINE Borrowed(const Borrowed& other) : ptr{other.ptr} {
    }
    PLY_INLINE void operator=(const Borrowed& other) {
        this->ptr = other.ptr;
    }
    PLY_INLINE void operator=(T* ptr) {
        this->ptr = ptr;
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
};

template <typename T>
class Owned {
private:
    T* ptr;

public:
    PLY_INLINE Owned() : ptr{nullptr} {
    }
    PLY_INLINE Owned(T* ptr) : ptr{ptr} { // FIXME: Replace with Owned<T>::adopt()
    }
    PLY_INLINE Owned(Owned&& other) : ptr{other.release()} {
    }
    template <typename Derived,
              typename std::enable_if_t<std::is_base_of<T, Derived>::value, int> = 0>
    PLY_INLINE Owned(Owned<Derived>&& other) : ptr{other.release()} {
    }
    PLY_INLINE ~Owned() {
        subst::destroyByMember(this->ptr);
    }
    static PLY_INLINE Owned adopt(T* ptr) {
        Owned result;
        result.ptr = ptr;
        return result;
    }
    PLY_INLINE void operator=(Owned&& other) {
        PLY_ASSERT(!this->ptr || this->ptr != other.ptr);
        subst::destroyByMember(this->ptr);
        this->ptr = other.release();
    }
    template <typename Derived,
              typename std::enable_if_t<std::is_base_of<T, Derived>::value, int> = 0>
    PLY_INLINE void operator=(Owned<Derived>&& other) {
        PLY_ASSERT(!this->ptr || this->ptr != other.ptr);
        subst::destroyByMember(this->ptr);
        this->ptr = other.release();
    }
    PLY_INLINE void operator=(T* ptr) {
        PLY_ASSERT(!this->ptr || this->ptr != ptr);
        subst::destroyByMember(this->ptr);
        this->ptr = ptr;
    }
    template <typename... Args>
    static PLY_INLINE Owned create(Args&&... args) {
        return Owned::adopt(new T{std::forward<Args>(args)...});
    }
    PLY_INLINE T* operator->() const {
        return this->ptr;
    }
    PLY_INLINE operator T*() const {
        return this->ptr;
    }
    PLY_INLINE Borrowed<T> borrow() const {
        return this->ptr;
    }
    PLY_INLINE T* get() const {
        return this->ptr;
    }
    PLY_INLINE T* release() {
        T* ptr = this->ptr;
        this->ptr = nullptr;
        return ptr;
    }
    PLY_INLINE void clear() {
        subst::destroyByMember(this->ptr);
        this->ptr = nullptr;
    }
};

template <typename T>
PLY_INLINE Borrowed<T> borrow(T* ptr) {
    return ptr;
}

template <typename T>
class OptionallyOwned {
private:
    uptr ptrAndOwnedFlag = 0;

public:
    PLY_INLINE OptionallyOwned() = default;
    template <typename Derived,
              typename std::enable_if_t<std::is_base_of<T, Derived>::value, int> = 0>
    PLY_INLINE OptionallyOwned(Owned<Derived>&& other) {
        PLY_ASSERT((((uptr) other.get()) & 1) == 0);
        this->ptrAndOwnedFlag = (uptr) other.release() | 1;
    }
    PLY_INLINE OptionallyOwned(OptionallyOwned&& other) : ptrAndOwnedFlag{other.ptrAndOwnedFlag} {
        other.ptrAndOwnedFlag = 0;
    }
    template <typename Derived,
              typename std::enable_if_t<std::is_base_of<T, Derived>::value, int> = 0>
    PLY_INLINE OptionallyOwned(const Borrowed<Derived>& other)
        : ptrAndOwnedFlag{(uptr) other.get()} {
        PLY_ASSERT((((uptr) other.get()) & 1) == 0);
    }
    PLY_INLINE ~OptionallyOwned() {
        if (ptrAndOwnedFlag & 1) {
            subst::destroyByMember((T*) (this->ptrAndOwnedFlag & ~(uptr) 1));
        }
    }
    PLY_INLINE void operator=(OptionallyOwned&& other) {
        PLY_ASSERT(!this->ptrAndOwnedFlag || this->ptrAndOwnedFlag != other.ptrAndOwnedFlag);
        destruct(*this);
        new (this) OptionallyOwned{std::move(other)};
    }
    template <typename Base, typename = std::enable_if_t<std::is_base_of<Base, T>::value>>
    PLY_INLINE operator OptionallyOwned<Base>&() {
        return reinterpret_cast<OptionallyOwned<Base>&>(*this);
    }
    PLY_INLINE T* operator->() const {
        return (T*) (this->ptrAndOwnedFlag & ~(uptr) 1);
    }
    PLY_INLINE operator T*() const {
        return (T*) (this->ptrAndOwnedFlag & ~(uptr) 1);
    }
    PLY_INLINE T* get() const {
        return (T*) (this->ptrAndOwnedFlag & ~(uptr) 1);
    }
    PLY_INLINE bool isOwned() const {
        return (this->ptrAndOwnedFlag & 1) != 0;
    }
    PLY_INLINE T* release() {
        T* ptr = (T*) (this->ptrAndOwnedFlag & ~(uptr) 1);
        this->ptrAndOwnedFlag = 0;
        return ptr;
    }
};

} // namespace ply

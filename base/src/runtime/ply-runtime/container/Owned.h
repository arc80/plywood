/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime.h>

namespace ply {

template <typename T>
class Owned {
private:
    template<typename> friend class Owned;
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

} // namespace ply

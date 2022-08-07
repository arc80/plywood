/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>

namespace ply {

//------------------------------------------------------------------
// Any
//------------------------------------------------------------------
struct Any {
    uptr storage = 0;

    PLY_INLINE Any() = default;

    template <typename T, typename... Args>
    PLY_INLINE Any(T*, Args&&... args) {
        T* target = (T*) &storage;
        if (sizeof(T) > sizeof(storage)) {
            storage = (uptr) PLY_HEAP.alloc(sizeof(T));
            target = (T*) storage;
        }
        new (target) T{std::forward<Args>(args)...};
    }

    template <typename T>
    PLY_INLINE Any(T&& other) {
        if (sizeof(T) > sizeof(storage)) {
            storage = other.storage;
            other.storage = nullptr;
        } else {
            (T&) storage = std::move((T&) other.storage);
        }
    }

    template <typename T>
    PLY_INLINE T* get() {
        return (sizeof(T) <= sizeof(storage)) ? (T*) &storage : (T*) storage;
    }

    template <typename T>
    PLY_INLINE void destruct() {
        subst::destructByMember(get<T>());
        if (sizeof(T) > sizeof(storage)) {
            PLY_HEAP.free((void*) storage);
        }
    }
};

} // namespace ply

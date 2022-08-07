/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>

// clang-format off

// Choose default implementation if not already configured by ply_userconfig.h:
#if !defined(PLY_IMPL_ATOMIC_PATH)
    #define PLY_IMPL_ATOMIC_PATH "impl/Atomic_CPP11.h"
    #define PLY_IMPL_ATOMIC_TYPE ply::Atomic_CPP11
#endif

// Include the implementation:
#include PLY_IMPL_ATOMIC_PATH

// Alias it:
namespace ply {
    
template <typename T> class Atomic : public PLY_IMPL_ATOMIC_TYPE<T> {
public:
    Atomic() {
    }
    Atomic(T value) : PLY_IMPL_ATOMIC_TYPE<T>(value) {
    }
    ~Atomic() {
    }
};

} // namespace ply

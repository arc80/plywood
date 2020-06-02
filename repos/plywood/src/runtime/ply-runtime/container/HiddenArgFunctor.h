/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>

namespace ply {

template <typename>
struct HiddenArgFunctor;

template <typename Return, typename... Args>
struct HiddenArgFunctor<Return(Args...)> {
    void* arg0 = nullptr;
    Return (*fn)(void*, Args...) = nullptr;

    PLY_INLINE HiddenArgFunctor() = default;
    template <typename T>
    PLY_INLINE HiddenArgFunctor(T* arg0, Return (*fn)(T*, Args...))
        : arg0{arg0}, fn{(Return(*)(void*, Args...)) fn} {
    }
    template <typename T>
    PLY_INLINE HiddenArgFunctor(T* arg0, Return (*fn)(const T*, Args...))
        : arg0{arg0}, fn{(Return(*)(void*, Args...)) fn} {
    }
    PLY_INLINE explicit operator bool() const {
        return this->fn != nullptr;
    }
    PLY_INLINE Return operator()(Args... args) const {
        PLY_ASSERT(this->fn);
        return this->fn(arg0, std::forward<Args>(args)...);
    }
};

} // namespace ply

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
    template <typename T>
    using FnType = Return(T*, Args...);
    void* arg0 = nullptr;
    FnType<void>* fn = nullptr;

    PLY_INLINE HiddenArgFunctor() = default;
    template <typename T>
    PLY_INLINE HiddenArgFunctor(T* arg0, FnType<T>* fn) : arg0{arg0}, fn{(FnType<void>*) fn} {
    }
    template <typename T>
    PLY_INLINE HiddenArgFunctor(T* arg0, FnType<const T>* fn) : arg0{arg0}, fn{(FnType<void>*) fn} {
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

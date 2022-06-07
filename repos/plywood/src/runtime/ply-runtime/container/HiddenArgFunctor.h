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
    Return (*fn)(void*, Args...) = nullptr;
    void* hiddenArg = nullptr;

    PLY_INLINE HiddenArgFunctor() = default;

    template <typename Callable, typename HiddenArgType>
    PLY_INLINE HiddenArgFunctor(const Callable& callable, HiddenArgType* hiddenArg)
        : hiddenArg{(void*) hiddenArg} {
        // We typically pass a lambda expression as `callable`. The lambda expression must be
        // implicitly convertible to a function pointer that can take `hiddenArg` as its first
        // argument; otherwise compilation will fail on this line.
        auto* fnPtr = static_cast<Return (*)(HiddenArgType*, Args...)>(callable);

        // We then C-style cast this function pointer to a different type in order to store it in
        // a member variable.
        this->fn = (decltype(this->fn)) fnPtr;
    }

    PLY_INLINE explicit operator bool() const {
        return this->fn != nullptr;
    }

    PLY_INLINE Return operator()(Args... args) const {
        PLY_ASSERT(this->fn);
        // The constructor guarantees that `fn` is compatible with `hiddenArg` even though we
        // discarded some type information in order to store them in member variables.
        return this->fn(hiddenArg, std::forward<Args>(args)...);
    }
};

} // namespace ply

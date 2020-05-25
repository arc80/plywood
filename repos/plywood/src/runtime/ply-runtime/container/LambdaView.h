/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>

namespace ply {

template <typename>
struct LambdaView;

template <typename Return, typename... Args>
struct LambdaView<Return(Args...)> {
    const void* callable = nullptr;
    Return (*thunk)(const void*, Args...) = nullptr;

    template <typename Callable>
    static PLY_NO_INLINE Return thunkImpl(const void* callable, Args... args) {
        return (*(const Callable*) callable)(std::forward<Args>(args)...);
    }

    PLY_INLINE LambdaView() = default;

    template <
        typename Callable,
        std::enable_if_t<std::is_same<decltype(std::declval<Callable>()(std::declval<Args>()...)),
                                      Return>::value,
                         int> = 0>
    PLY_INLINE LambdaView(const Callable& callable) : callable{&callable} {
        thunk = thunkImpl<Callable>;
    }

    PLY_INLINE Return operator()(Args... args) const {
        return this->thunk(this->callable, std::forward<Args>(args)...);
    }
};

} // namespace ply

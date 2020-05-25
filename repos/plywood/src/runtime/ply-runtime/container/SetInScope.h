/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>

namespace ply {

//------------------------------------------------------------
// SetInScope
//------------------------------------------------------------
template <typename T, typename V>
struct SetInScope {
    T& target;                           // The variable to set/reset
    std::remove_reference_t<T> oldValue; // Backup of original value
    const V& newValueRef; // Extends the lifetime of temporary values in the case of eg.
                          // SetInScope<StringView, String>

    template <typename U>
    SetInScope(T& target, U&& newValue)
        : target{target}, oldValue{std::move(target)}, newValueRef{newValue} {
        target = std::forward<U>(newValue);
    }
    ~SetInScope() {
        this->target = std::move(this->oldValue);
    }
};

#define PLY_SET_IN_SCOPE(target, value) \
    SetInScope<decltype(target), decltype(value)> PLY_UNIQUE_VARIABLE(setInScope) { \
        target, value \
    }

//------------------------------------------------------------
// OnScopeExit
//------------------------------------------------------------
template <typename Callback>
struct OnScopeExit {
    Callback cb;
    PLY_INLINE ~OnScopeExit() {
        cb();
    }
};
template <typename Callback>
PLY_INLINE OnScopeExit<Callback> setOnScopeExit(Callback&& cb) {
    return {std::forward<Callback>(cb)};
}
#define PLY_ON_SCOPE_EXIT(cb) auto PLY_UNIQUE_VARIABLE(onScopeExit) = setOnScopeExit([&] cb)

} // namespace ply

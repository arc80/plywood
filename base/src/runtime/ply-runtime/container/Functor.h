/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <utility>

namespace ply {

template <typename>
struct Functor;

template <typename Return, typename... Args>
class Functor<Return(Args...)> {
private:
    typedef Return Handler(void*, Args...);

    Handler* handler = nullptr;
    void* storedArg = nullptr;

public:
    Functor() = default;

    PLY_INLINE Functor(const Functor& other) : handler{other.handler}, storedArg{other.storedArg} {
    }

    template <typename T>
    PLY_INLINE Functor(Return (*handler)(T*, Args...), T* storedArg)
        : handler{(Handler*) handler}, storedArg{(void*) storedArg} {
    }

    template <typename T>
    PLY_INLINE Functor(Return (T::*handler)(Args...), T* target) : storedArg{(void*) target} {
        this->handler = [this](void* target, Args... args) {
            return ((T*) target)->*(this->hiddenArg)(std::forward<Args>(args)...);
        };
    }

    // Support lambda expressions
    template <typename Callable,
              typename = void_t<decltype(std::declval<Callable>()(std::declval<Args>()...))>>
    Functor(const Callable& callable) : storedArg{(void*) &callable} {
        this->handler = [](void* callable, Args... args) -> Return {
            return (*(const Callable*) callable) (std::forward<Args>(args)...);
        };
    }

    PLY_INLINE void operator=(const Functor& other) {
        this->handler = other.handler;
        this->storedArg = other.storedArg;
    }

    PLY_INLINE explicit operator bool() const {
        return this->handler != nullptr;
    }

    template <typename... CallArgs>
    PLY_INLINE Return operator()(CallArgs&&... args) const {
        if (!this->handler)
            return subst::createDefault<Return>();
        PLY_PUN_SCOPE
        return this->handler(this->storedArg, std::forward<CallArgs>(args)...);
    }
};

} // namespace ply

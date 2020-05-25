/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>

namespace ply {

template <typename>
struct Functor;

template <typename Return, typename... Args>
struct Functor<Return(Args...)> {
private:
    struct BaseWrapper {
        Return (*call)(BaseWrapper* wrapper, Args... args) = nullptr;
    };

    template <typename Invocable>
    struct Wrapper : BaseWrapper {
        std::decay_t<Invocable> inv;

        static PLY_NO_INLINE Return call(BaseWrapper* wrapper, Args... args) {
            return static_cast<Wrapper*>(wrapper)->inv(std::forward<Args>(args)...);
        }
        template <typename I>
        PLY_INLINE Wrapper(I&& inv) : BaseWrapper{&call}, inv{std::forward<I>(inv)} {
        }
    };

    BaseWrapper* wrapper = nullptr;

public:
    PLY_INLINE Functor() = default;
    PLY_INLINE Functor(Functor&& other) : wrapper{other.wrapper} {
        other.wrapper = nullptr;
    }
    PLY_INLINE bool isValid() const {
        return this->wrapper != nullptr;
    }
    PLY_INLINE explicit operator bool() const {
        return this->wrapper != nullptr;
    }
    PLY_INLINE ~Functor() {
        if (this->wrapper) {
            delete this->wrapper;
        }
    }
    PLY_INLINE void operator=(Functor&& other) {
        this->wrapper = other.wrapper;
        other.wrapper = nullptr;
    }
    PLY_INLINE Return call(Args... args) const {
        return this->wrapper->call(this->wrapper, std::forward<Args>(args)...);
    }
    template <typename I, typename = void_t<decltype(std::declval<I>()(std::declval<Args>()...))>>
    PLY_INLINE Functor(I&& inv) : wrapper{new Wrapper<I>{std::forward<I>(inv)}} {
    }
};

} // namespace ply

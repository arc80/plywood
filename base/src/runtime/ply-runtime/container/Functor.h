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
        Return (*thunk)(BaseWrapper* wrapper, Args... args) = nullptr;
    };

    template <typename Callable>
    struct Wrapper : BaseWrapper {
        std::decay_t<Callable> callable;

        static PLY_NO_INLINE Return thunk(BaseWrapper* wrapper, Args... args) {
            return static_cast<Wrapper*>(wrapper)->callable(std::forward<Args>(args)...);
        }
        template <typename I>
        PLY_INLINE Wrapper(I&& callable)
            : BaseWrapper{&thunk}, callable{std::forward<I>(callable)} {
        }
    };

    BaseWrapper* wrapper = nullptr;

public:
    PLY_INLINE Functor() = default;
    PLY_INLINE Functor(Functor&& other) : wrapper{other.wrapper} {
        other.wrapper = nullptr;
    }
    template <typename Callable,
              typename = void_t<decltype(std::declval<Callable>()(std::declval<Args>()...))>>
    PLY_INLINE Functor(Callable&& callable)
        : wrapper{new Wrapper<Callable>{std::forward<Callable>(callable)}} {
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
    PLY_INLINE bool isValid() const {
        return this->wrapper != nullptr;
    }
    PLY_INLINE explicit operator bool() const {
        return this->wrapper != nullptr;
    }
    PLY_INLINE Return operator()(Args... args) const {
        return this->wrapper->thunk(this->wrapper, std::forward<Args>(args)...);
    }
};

} // namespace ply

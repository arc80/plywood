/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <thread>
#include <chrono>

namespace ply {

#define PLY_THREAD_STARTCALL

class Thread_CPP11 {
protected:
    std::thread thread;

public:
    typedef void* ReturnType;
    typedef void* (*StartRoutine)(void*);

    PLY_INLINE Thread_CPP11() = default;

    template <typename Callable>
    PLY_INLINE Thread_CPP11(Callable&& callable) : thread{std::forward<Callable>(callable)} {
    }

    PLY_INLINE ~Thread_CPP11() {
        if (this->thread.joinable())
            this->thread.detach();
    }

    PLY_INLINE bool isValid() const {
        return this->thread.joinable();
    }

    PLY_INLINE void join() {
        this->thread.join();
    }

    template <typename Callable>
    PLY_INLINE void run(Callable&& callable) {
        if (this->thread.joinable())
            this->thread.detach();
        this->thread = std::thread(std::forward<Callable>(callable));
    }

    static PLY_INLINE void sleepMillis(ureg millis) {
        std::this_thread::sleep_for(std::chrono::milliseconds(millis));
    }
};

} // namespace ply

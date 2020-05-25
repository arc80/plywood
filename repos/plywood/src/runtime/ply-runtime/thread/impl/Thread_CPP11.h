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
    std::thread m_thread;

public:
    typedef void* ReturnType;
    typedef void* (*StartRoutine)(void*);

    Thread_CPP11() {
    }

    Thread_CPP11(StartRoutine startRoutine, void* arg = NULL) : m_thread(startRoutine, arg) {
    }

    ~Thread_CPP11() {
        if (m_thread.joinable())
            m_thread.detach();
    }

    bool isValid() const {
        return m_thread.joinable();
    }

    void join() {
        m_thread.join();
    }

    void run(StartRoutine startRoutine, void* arg = NULL) {
        if (m_thread.joinable())
            m_thread.detach();
        m_thread = std::thread(startRoutine, arg);
    }

    static void sleepMillis(ureg millis) {
        std::this_thread::sleep_for(std::chrono::milliseconds(millis));
    }
};

} // namespace ply

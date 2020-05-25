/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/thread/impl/Mutex_CPP11.h>
#include <condition_variable>

namespace ply {

class ConditionVariable_CPP11 {
private:
    std::condition_variable_any m_condVar;

public:
    void wait(ply::LockGuard<Mutex_CPP11>& guard) {
        m_condVar.wait(guard);
    }

    void timedWait(ply::LockGuard<Mutex_CPP11>& guard, ureg waitMillis) {
        if (waitMillis > 0)
            m_condVar.wait_for(guard, std::chrono::milliseconds(waitMillis));
    }

    void wakeOne() {
        m_condVar.notify_one();
    }

    void wakeAll() {
        m_condVar.notify_all();
    }
};

} // namespace ply

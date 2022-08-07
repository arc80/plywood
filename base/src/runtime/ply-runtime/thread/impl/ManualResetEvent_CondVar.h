/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/thread/ConditionVariable.h>

namespace ply {

class ManualResetEvent_CondVar {
private:
    ply::Mutex m_mutex;
    ply::ConditionVariable m_condVar;
    bool m_state;

public:
    ManualResetEvent_CondVar(bool initialState = false) : m_state(initialState) {
    }

    ~ManualResetEvent_CondVar() {
    }

    void signal() {
        ply::LockGuard<ply::Mutex> guard(m_mutex);
        m_state = true;
        m_condVar.wakeAll();
    }

    void reset() {
        ply::LockGuard<ply::Mutex> guard(m_mutex);
        m_state = false;
    }

    void wait() {
        ply::LockGuard<ply::Mutex> guard(m_mutex);
        while (!m_state) {
            m_condVar.wait(guard);
        }
    }
};

} // namespace ply

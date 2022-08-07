/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <mach/mach.h>

namespace ply {

class Semaphore_Mach {
private:
    semaphore_t m_semaphore;

public:
    Semaphore_Mach() {
        semaphore_create(mach_task_self(), &m_semaphore, SYNC_POLICY_FIFO, 0);
    }

    ~Semaphore_Mach() {
        semaphore_destroy(mach_task_self(), m_semaphore);
    }

    void wait() {
        semaphore_wait(m_semaphore);
    }

    void signal(ureg count = 1) {
        while (count-- > 0)
            semaphore_signal(m_semaphore);
    }
};

} // namespace ply

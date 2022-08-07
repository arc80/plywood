/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#if PLY_KERNEL_MACH
// Mach can't use POSIX semaphores due to
// http://lists.apple.com/archives/darwin-kernel/2009/Apr/msg00010.html
#error "Can't use unnamed POSIX semaphores on Mach."
#endif
#include <semaphore.h>
#include <errno.h>

namespace ply {

class Semaphore_POSIX {
private:
    sem_t m_sem;

public:
    Semaphore_POSIX() {
        sem_init(&m_sem, 0, 0);
    }

    ~Semaphore_POSIX() {
        sem_destroy(&m_sem);
    }

    void wait() {
        // http://stackoverflow.com/questions/2013181/gdb-causes-sem-wait-to-fail-with-eintr-error
        int rc;
        do {
            rc = sem_wait(&m_sem);
        } while (rc == -1 && errno == EINTR);
    }

    void signal(ureg count = 1) {
        while (count-- > 0)
            sem_post(&m_sem);
    }
};

} // namespace ply

/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-platform/Util.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#ifdef PLY_KERNEL_FREEBSD
#include <pthread_np.h>
#endif

namespace ply {

class TID_POSIX {
public:
#ifdef PLY_TARGET_MINGW
    typedef uptr TID;
#else
    // This only works when pthread_t is an integer type, as it is in the GNU C Library
    // >= 2.3.3. If that's not true for your Pthreads library, we'll need to extend Plywood to
    // fetch TIDs from somewehere else in the environment.
    using TID = SizedInt<sizeof(pthread_t)>::Unsigned;
#endif
    using PID = SizedInt<sizeof(pid_t)>::Unsigned;

    static TID getCurrentThreadID() {
        // FIXME: On Linux, would the kernel task ID be more useful for debugging?
        // If so, detect NPTL at compile time and create TID_NPTL.h which uses gettid()
        // instead.
#ifdef PLY_KERNEL_FREEBSD
        return pthread_getthreadid_np();
#elif PLY_TARGET_MINGW
        return (TID) pthread_self().p;
#else
        return pthread_self();
#endif
    }

    static PID getCurrentProcessID() {
        return getpid();
    }
};

} // namespace ply

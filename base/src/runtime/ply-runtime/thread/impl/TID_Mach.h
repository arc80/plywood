/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-platform/Util.h>
#include <pthread.h>
#include <mach/mach_types.h>
#include <unistd.h>

namespace ply {

class TID_Mach {
public:
    using TID = SizedInt<sizeof(thread_port_t)>::Unsigned;
    using PID = SizedInt<sizeof(pid_t)>::Unsigned;

    static TID getCurrentThreadID() {
        return pthread_mach_thread_np(pthread_self());
    }

    static PID getCurrentProcessID() {
        return getpid();
    }
};

} // namespace ply

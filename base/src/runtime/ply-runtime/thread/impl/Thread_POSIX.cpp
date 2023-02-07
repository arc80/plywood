﻿/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-runtime/Precomp.h>

#if PLY_TARGET_POSIX

#include <ply-runtime/thread/impl/Thread_POSIX.h>
#include <errno.h>

namespace ply {

#if !PLY_TARGET_MINGW
void Thread_POSIX::sleep_millis(ureg millis) {
    timespec ts;
    ts.tv_sec = millis / 1000;
    ts.tv_nsec = (millis % 1000) * 1000000;
    int rc;
    do {
        rc = nanosleep(&ts, NULL);
        PLY_ASSERT(rc == 0 || (rc == -1 && errno == EINTR));
    } while (rc == -1 && errno == EINTR);
}
#endif

} // namespace ply

#endif // PLY_TARGET_POSIX

/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-runtime/Precomp.h>

#if !PLY_DLL_IMPORTING

#include <ply-runtime/time/UTCTime.h>

#if PLY_TARGET_WIN32

namespace ply {
uint64_t getCurrentUTCTime() {
    FILETIME fileTime;
    ULARGE_INTEGER largeInteger;

    GetSystemTimeAsFileTime(&fileTime);
    largeInteger.LowPart = fileTime.dwLowDateTime;
    largeInteger.HighPart = fileTime.dwHighDateTime;
    return largeInteger.QuadPart / 10;
}
} // namespace ply

#elif PLY_TARGET_POSIX

#if PLY_USE_POSIX_2008_CLOCK
#include <time.h>
#else
#include <sys/time.h>
#endif

namespace ply {
uint64_t getCurrentUTCTime() {
#if PLY_USE_POSIX_2008_CLOCK
    struct timespec tick;
    clock_gettime(CLOCK_REALTIME, &tick);
    return (uint64_t) tick.tv_sec * 1000000ull + tick.tv_nsec / 1000 + 11644473600000000ull;
#else
    struct timeval tick;
    gettimeofday(&tick, NULL);
    return (uint64_t) tick.tv_sec * 1000000ull + tick.tv_usec + 11644473600000000ull;
#endif
}
} // namespace ply

#endif

#endif // !PLY_DLL_IMPORTING

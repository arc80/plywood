/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-platform/Util.h>

namespace ply {

class TID_Win32 {
public:
    using TID = SizedInt<sizeof(DWORD)>::Unsigned;
    using PID = SizedInt<sizeof(DWORD)>::Unsigned;

    // clang-format off
    static TID getCurrentThreadID() {
#if PLY_CPU_X64                                 // Windows x64
        return ((DWORD*) __readgsqword(48))[18]; // Read directly from the TIB
#elif PLY_CPU_X86 // Windows x86
        return ((DWORD*) __readfsdword(24))[9]; // Read directly from the TIB
#else
        return GetCurrentThreadID();
#endif
    }

    static PID getCurrentProcessID() {
#if PLY_CPU_X64                                 // Windows x64
        return ((DWORD*) __readgsqword(48))[16]; // Read directly from the TIB
#elif PLY_CPU_X86         // Windows x86
        return ((DWORD*) __readfsdword(24))[8]; // Read directly from the TIB
#elif PLY_TARGET_XBOX_360 // Xbox 360
        return 0;
#else
        return GetCurrentProcessID();
#endif
    }
    // clang-format on
};

} // namespace ply

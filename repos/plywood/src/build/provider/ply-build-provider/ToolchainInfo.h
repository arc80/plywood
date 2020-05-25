/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-build-common/Core.h>

namespace ply {
namespace build {

//--------------------------------------------
// ToolchainInfo
// Provides information about the toolchain used in a given build folder.
// Used to determine compatible extern providers.
//--------------------------------------------
struct ToolchainInfo {
    struct Compiler {
        String name = "unknown"; // "msvc", "gcc", "clang", ...
        u32 major = 0;
        u32 minor = 0;
    };
    struct Platform {
        String name = "unknown"; // "windows", "linux", "macos", "ios", "cygwin", "mingw", ...
        u32 major = 0;
        u32 minor = 0;
    };
    struct CRT {
        String name = "unknown"; // "msvc", "glibc", "newlib", ...
        bool shared = false;     // true if DLL
        u32 major = 0;
        u32 minor = 0;
    };
    struct CppRT {
        String name = "unknown"; // "msvc", "libstdc++", "libc++", ...
        bool shared = false;     // true if DLL
        u32 major = 0;
        u32 minor = 0;
    };

    Compiler compiler;
    Platform targetPlatform;
    String arch = "unknown"; // "x86", "x64", "arm64", ...
    CRT crt;
    CppRT cpprt;
};

} // namespace build
} // namespace ply

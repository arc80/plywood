/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once

// clang-format off

#if defined(_MSC_VER)
    // Win32 API
    #define PLY_COMPILER_MSVC 1
    #define PLY_TARGET_WIN32 1
    #define PLY_LONG_SIZE 4
    #define PLY_IS_BIG_ENDIAN 0
    #ifndef PLY_HAS_STDINT
        #if _MSC_VER >= 1600
            #define PLY_HAS_STDINT 1
        #endif
    #endif
    #if defined(_M_X64)
        // x64
        #define PLY_CPU_X64 1
        #define PLY_PTR_SIZE 8
    #elif defined(_M_IX86)
        // x86
        #define PLY_CPU_X86 1
        #define PLY_PTR_SIZE 4
    #else
        #error "Unrecognized platform!"
    #endif

#elif defined(__GNUC__)
    // GCC compiler family
    #define PLY_COMPILER_GCC 1
    #ifndef PLY_HAS_STDINT
        #define PLY_HAS_STDINT 1
    #endif
    #if defined(__APPLE__)
        #define PLY_TARGET_APPLE 1
        #define PLY_TARGET_POSIX 1
        #include <TargetConditionals.h>
        #if TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE
            #define PLY_TARGET_IOS 1
        #endif
    #endif
    #if defined(__FreeBSD__)
        #define PLY_TARGET_POSIX 1
        #define PLY_KERNEL_FREEBSD 1
    #endif
    #if defined(__linux__)
        #define PLY_TARGET_POSIX 1
        #define PLY_KERNEL_LINUX 1
        #if defined(__ANDROID__)
            #define PLY_TARGET_ANDROID 1
        #endif
    #endif
    #if defined(__MACH__)
        // Mach kernel, eg. Apple MacOS/iOS
        #define PLY_KERNEL_MACH 1
    #endif
    #if defined(__MINGW32__) || defined(__MINGW64__)
        // Leave PLY_TARGET_WIN32 undefined because too much API is missing from MinGW's Win32 implementation
        #define PLY_TARGET_MINGW 1
        #define PLY_TARGET_POSIX 1
    #endif
    #if defined(__x86_64__)
        // x64
        #define PLY_CPU_X64 1
        #define PLY_PTR_SIZE 8
    #elif defined(__i386__)
        // x86
        #define PLY_CPU_X86 1
        #define PLY_PTR_SIZE 4
    #elif defined(__arm__)
        // ARM
        #define PLY_CPU_ARM 1
        #define PLY_PTR_SIZE 4
        #if defined(__ARM_ARCH_7__) || defined(__ARM_ARCH_7A__) || defined(__ARM_ARCH_7EM__) || defined(__ARM_ARCH_7R__) || defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7S__)
            // ARMv7
            #define PLY_CPU_ARM_VERSION 7
        #elif defined(__ARM_ARCH_6__) || defined(__ARM_ARCH_6J__) || defined(__ARM_ARCH_6K__) || defined(__ARM_ARCH_6T2__) || defined(__ARM_ARCH_6Z__) || defined(__ARM_ARCH_6ZK__)
            // ARMv6
            #define PLY_CPU_ARM_VERSION 6
        #else
            // Could support earlier ARM versions at some point using compiler barriers and swp instruction
            #error "Unrecognized ARM CPU architecture version!"
        #endif
        #if defined(__thumb__)
            // Thumb instruction set mode
            #define PLY_CPU_ARM_THUMB 1
        #endif
    #elif defined(__arm64__) || defined(__aarch64__)
        // ARM64
        #define PLY_CPU_ARM64 1
        #define PLY_PTR_SIZE 8
    #elif defined(__powerpc__) || defined(__POWERPC__) || defined(__PPC__)
        #define PLY_CPU_POWERPC 1
        #if defined(__powerpc64__)
          #define PLY_PTR_SIZE 8
        #else
          #define PLY_PTR_SIZE 4 // 32-bit architecture
        #endif
    #else
        #error "Unrecognized target CPU!"
    #endif
    #define PLY_LONG_SIZE PLY_PTR_SIZE
    #if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
        #define PLY_IS_BIG_ENDIAN 1
    #else
        #define PLY_IS_BIG_ENDIAN 0
    #endif

#else
    #error "Unrecognized compiler!"
#endif

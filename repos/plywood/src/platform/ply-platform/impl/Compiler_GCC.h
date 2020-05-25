/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once

//-------------------------------------
//  Alignment
//-------------------------------------
// Note: May not work on local variables.
// http://gcc.gnu.org/bugzilla/show_bug.cgi?id=24691
#define PLY_DECL_ALIGNED(declaration, amt) declaration __attribute__((aligned(amt)))

//-------------------------------------
//  Inlining
//-------------------------------------
#define PLY_INLINE inline __attribute__((always_inline))
#define PLY_NO_INLINE __attribute__((noinline))

//-------------------------------------
//  Thread local
//-------------------------------------
#define PLY_THREAD_LOCAL __thread

//-------------------------------------
//  CPU intrinsics
//-------------------------------------
PLY_INLINE void ply_yieldHWThread() {
#if PLY_CPU_X86 || PLY_CPU_X64
    // Only implemented on x86/64
    asm volatile("pause");
#endif
}

#define PLY_DEBUG_BREAK() __builtin_trap()
#define PLY_FORCE_CRASH() __builtin_trap()

//-------------------------------------
//  DLL imports
//-------------------------------------
#define PLY_DLL_IMPORT
#define PLY_DLL_EXPORT

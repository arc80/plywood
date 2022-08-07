/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <intrin.h>

//-------------------------------------
//  Alignment
//-------------------------------------
#define PLY_DECL_ALIGNED(declaration, amt) __declspec(align(amt)) declaration

//-------------------------------------
//  Inlining
//-------------------------------------
#define PLY_INLINE __forceinline
#define PLY_NO_INLINE __declspec(noinline)

//-------------------------------------
//  Thread local
//-------------------------------------
#define PLY_THREAD_LOCAL __declspec(thread)

//-------------------------------------
//  Debug break
//-------------------------------------
#define PLY_DEBUG_BREAK() __debugbreak()
#define PLY_FORCE_CRASH() __ud2()
#define PLY_COMPILER_BARRIER() _ReadWriteBarrier()

PLY_INLINE void ply_yieldHWThread() {
    YieldProcessor();
}

//-------------------------------------
//  DLL imports
//-------------------------------------
#define PLY_DLL_IMPORT __declspec(dllimport)
#define PLY_DLL_EXPORT __declspec(dllexport)

//-------------------------------------
//  nodiscard
//-------------------------------------
#if _MSC_VER >= 1700
#define PLY_NO_DISCARD _Check_return_
#else
#define PLY_NO_DISCARD
#endif

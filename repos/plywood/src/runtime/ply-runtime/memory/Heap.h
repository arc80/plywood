/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>

// clang-format off

// Choose implementation based on ply_config.h:
#if PLY_USE_DLMALLOC
    #define PLY_IMPL_HEAP_PATH "impl/Heap_DL.h"
    #define PLY_IMPL_HEAP_TYPE ply::Heap_DL
#else
    #define PLY_IMPL_HEAP_PATH "impl/Heap_CRT.h"
    #define PLY_IMPL_HEAP_TYPE ply::Heap_CRT
#endif

// Include the implementation:
#include PLY_IMPL_HEAP_PATH

// Alias it:
PLY_DLL_ENTRY extern PLY_IMPL_HEAP_TYPE PlyHeap;

#define PLY_HEAP_DIRECT(heap) (heap).operate(__FILE__ "(" PLY_STRINGIFY(__LINE__) ")")
#define PLY_HEAP PLY_HEAP_DIRECT(PlyHeap)

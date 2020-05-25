/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Base.h>
#include <ply-reflect/TypeDescriptor.h>

#if PLY_BUILD_IMPORTING
#define PLY_BUILD_ENTRY PLY_DLL_IMPORT
#elif PLY_BUILD_EXPORTING
#define PLY_BUILD_ENTRY PLY_DLL_EXPORT
#else
#define PLY_BUILD_ENTRY
#endif

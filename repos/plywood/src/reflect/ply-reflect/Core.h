/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Base.h>

#define PLY_WITH_METHOD_TABLES 1

#if PLY_WITH_METHOD_TABLES
#define PLY_METHOD_TABLES_ONLY(...) __VA_ARGS__
#else
#define PLY_METHOD_TABLES_ONLY(x)
#endif

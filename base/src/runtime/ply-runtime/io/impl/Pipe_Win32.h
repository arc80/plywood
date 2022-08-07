/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>

#if !PLY_TARGET_WIN32
#error "File should not be included"
#endif

#include <ply-runtime/io/Pipe.h>

namespace ply {

//------------------------------------------------------------------
// InPipe_Win32
//------------------------------------------------------------------
struct InPipe_Win32 : InPipe {
    static Funcs Funcs_;
    HANDLE handle = INVALID_HANDLE_VALUE;

    PLY_DLL_ENTRY InPipe_Win32(HANDLE handle);
};

//------------------------------------------------------------------
// OutPipe_Win32
//------------------------------------------------------------------
struct OutPipe_Win32 : OutPipe {
    static Funcs Funcs_;
    HANDLE handle = INVALID_HANDLE_VALUE;

    PLY_DLL_ENTRY OutPipe_Win32(HANDLE handle);
};

} // namespace ply

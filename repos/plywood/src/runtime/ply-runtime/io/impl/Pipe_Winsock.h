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
#include <ply-runtime/container/BufferView.h>
#include <winsock2.h>

namespace ply {

//------------------------------------------------------------------
// InPipe_Winsock
//------------------------------------------------------------------
struct InPipe_Winsock : InPipe {
    static Funcs Funcs_;
    SOCKET socket = INVALID_SOCKET;

    PLY_DLL_ENTRY InPipe_Winsock(SOCKET socket);
};

//------------------------------------------------------------------
// OutPipe_Winsock
//------------------------------------------------------------------
struct OutPipe_Winsock : OutPipe {
    static Funcs Funcs_;
    SOCKET socket = INVALID_SOCKET;

    PLY_DLL_ENTRY OutPipe_Winsock(SOCKET socket);
};

} // namespace ply

/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/io/Pipe.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/socket.h>

namespace ply {

//------------------------------------------------------------------
// InPipe_FD
//------------------------------------------------------------------
struct InPipe_FD : InPipe {
    static Funcs Funcs_;
    int fd = -1;

    PLY_DLL_ENTRY InPipe_FD(int fd);
};

//------------------------------------------------------------------
// OutPipe_FD
//------------------------------------------------------------------
struct OutPipe_FD : OutPipe {
    static Funcs Funcs_;
    int fd = -1;

    PLY_DLL_ENTRY OutPipe_FD(int fd);
};

} // namespace ply

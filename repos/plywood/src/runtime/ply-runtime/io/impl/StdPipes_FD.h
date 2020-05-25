/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/io/Pipe.h>
#include <ply-runtime/container/Owned.h>

namespace ply {

struct StdPipes_FD {
    static PLY_DLL_ENTRY Borrowed<InPipe> stdIn();
    static PLY_DLL_ENTRY Borrowed<OutPipe> stdOut();
    static PLY_DLL_ENTRY Borrowed<OutPipe> stdErr();
};

} // namespace ply

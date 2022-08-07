/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-runtime/Precomp.h>

#if PLY_TARGET_POSIX

#include <ply-runtime/io/impl/StdPipes_FD.h>
#include <ply-runtime/io/impl/Pipe_FD.h>

namespace ply {

PLY_NO_INLINE Borrowed<InPipe> StdPipes_FD::stdIn() {
    // never destructed
    static InPipe_FD inPipe{STDIN_FILENO};
    return &inPipe;
}

PLY_NO_INLINE Borrowed<OutPipe> StdPipes_FD::stdOut() {
    // never destructed
    static OutPipe_FD outPipe{STDOUT_FILENO};
    return &outPipe;
}

PLY_NO_INLINE Borrowed<OutPipe> StdPipes_FD::stdErr() {
    // never destructed
    static OutPipe_FD outPipe{STDERR_FILENO};
    return &outPipe;
}

} // namespace ply

#endif // PLY_TARGET_POSIX

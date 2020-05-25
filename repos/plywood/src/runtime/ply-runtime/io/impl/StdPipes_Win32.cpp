/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-runtime/Precomp.h>

#if PLY_TARGET_WIN32

#include <ply-runtime/io/impl/StdPipes_Win32.h>
#include <ply-runtime/io/impl/Pipe_Win32.h>

namespace ply {

PLY_NO_INLINE Borrowed<InPipe> StdPipes_Win32::stdIn() {
    // never destructed
    static InPipe_Win32 inPipe{GetStdHandle(STD_INPUT_HANDLE)};
    return &inPipe;
}

PLY_NO_INLINE Borrowed<OutPipe> StdPipes_Win32::stdOut() {
    // never destructed
    static OutPipe_Win32 outPipe{GetStdHandle(STD_OUTPUT_HANDLE)};
    return &outPipe;
}

PLY_NO_INLINE Borrowed<OutPipe> StdPipes_Win32::stdErr() {
    // never destructed
    static OutPipe_Win32 outPipe{GetStdHandle(STD_ERROR_HANDLE)};
    return &outPipe;
}

} // namespace ply

#endif // PLY_TARGET_WIN32

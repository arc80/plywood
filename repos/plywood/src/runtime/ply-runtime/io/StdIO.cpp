/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-runtime/Precomp.h>
#include <ply-runtime/io/StdIO.h>
#include <ply-runtime/io/text/NewLineFilter.h>

namespace ply {

PLY_NO_INLINE InStream StdIn::text() {
    Owned<InStream> ins = new InStream{StdPipes::stdIn()};
    return InStream{createInNewLineFilter(std::move(ins))};
}

PLY_NO_INLINE OutStream StdOut::text() {
    Owned<OutStream> outs = new OutStream{StdPipes::stdOut()};
    bool writeCRLF = false;
#if PLY_TARGET_WIN32
    writeCRLF = true;
#endif
    return OutStream{createOutNewLineFilter(std::move(outs), writeCRLF)};
}

PLY_NO_INLINE OutStream StdErr::text() {
    Owned<OutStream> outs = new OutStream{StdPipes::stdErr()};
    bool writeCRLF = false;
#if PLY_TARGET_WIN32
    writeCRLF = true;
#endif
    return OutStream{createOutNewLineFilter(std::move(outs), writeCRLF)};
}

} // namespace ply

/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-runtime/Precomp.h>
#include <ply-runtime/io/StdIO.h>
#include <ply-runtime/io/text/NewLineFilter.h>

namespace ply {

PLY_NO_INLINE StringReader StdIn::text() {
    Owned<InStream> ins = new InStream{StdPipes::stdIn()};
    return StringReader{createInNewLineFilter(std::move(ins))};
}

PLY_NO_INLINE StringWriter StdOut::text() {
    Owned<OutStream> outs = new OutStream{StdPipes::stdOut()};
    bool writeCRLF = false;
#if PLY_TARGET_WIN32
    writeCRLF = true;
#endif
    return StringWriter{createOutNewLineFilter(std::move(outs), writeCRLF)};
}

PLY_NO_INLINE StringWriter StdErr::text() {
    Owned<OutStream> outs = new OutStream{StdPipes::stdErr()};
    bool writeCRLF = false;
#if PLY_TARGET_WIN32
    writeCRLF = true;
#endif
    return StringWriter{createOutNewLineFilter(std::move(outs), writeCRLF)};
}

} // namespace ply

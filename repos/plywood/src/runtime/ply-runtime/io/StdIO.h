/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/io/text/StringReader.h>
#include <ply-runtime/io/text/StringWriter.h>

#if !defined(PLY_IMPL_STDPIPES_PATH)
#if PLY_TARGET_WIN32
#define PLY_IMPL_STDPIPES_PATH "impl/StdPipes_Win32.h"
#define PLY_IMPL_STDPIPES_TYPE StdPipes_Win32
#define PLY_WITH_STDPIPES_WIN32 1
#elif PLY_TARGET_POSIX
#define PLY_IMPL_STDPIPES_PATH "impl/StdPipes_FD.h"
#define PLY_IMPL_STDPIPES_TYPE StdPipes_FD
#define PLY_WITH_STDPIPES_FD 1
#else
#define PLY_IMPL_STDPIPES_PATH "*** Unable to select a default StdPipes implementation ***"
#endif
#endif

#include PLY_IMPL_STDPIPES_PATH

namespace ply {

using StdPipes = PLY_IMPL_STDPIPES_TYPE;

struct StdIn {
    static PLY_INLINE Borrowed<InPipe> pipe() {
        return StdPipes::stdIn();
    }
    static PLY_INLINE InStream binary() {
        return InStream{StdPipes::stdIn()};
    }
    static PLY_DLL_ENTRY StringReader text();

    // FIXME: Remove these declarations later:
    static InStream createStream() = delete;       // call binary() instead
    static InStream createStringReader() = delete; // call text() instead
};

struct StdOut {
    static PLY_INLINE Borrowed<OutPipe> pipe() {
        return StdPipes::stdOut();
    }
    static PLY_INLINE OutStream binary() {
        return OutStream{StdPipes::stdOut()};
    }
    static PLY_DLL_ENTRY StringWriter text();

    // FIXME: Remove these declarations later:
    static InStream createStream() = delete;       // call binary() instead
    static InStream createStringWriter() = delete; // call text() instead
};

struct StdErr {
    static PLY_INLINE Borrowed<OutPipe> pipe() {
        return StdPipes::stdErr();
    }
    static PLY_INLINE OutStream binary() {
        return OutStream{StdPipes::stdErr()};
    }
    static PLY_DLL_ENTRY StringWriter text();

    // FIXME: Remove these declarations later:
    static InStream createStream() = delete;       // call binary() instead
    static InStream createStringWriter() = delete; // call text() instead
};

} // namespace ply

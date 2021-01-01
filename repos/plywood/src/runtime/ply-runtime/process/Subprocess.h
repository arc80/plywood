/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/container/Owned.h>
#include <ply-runtime/io/Pipe.h>
#include <ply-runtime/io/StdIO.h>
#include <ply-runtime/string/StringView.h>

namespace ply {

struct Subprocess {
    enum struct Pipe {
        Open,
        Redirect, // This will redirect output to /dev/null if corresponding OutPipe
                  // (stdOutPipe/stdErrPipe) is unopened
        StdOut,
    };

    struct Output {
        Pipe stdOut = Pipe::Redirect;
        Pipe stdErr = Pipe::Redirect;
        OutPipe* stdOutPipe = nullptr;
        OutPipe* stdErrPipe = nullptr;

        static PLY_INLINE Output ignore() {
            return {};
        }
        static PLY_INLINE Output inherit() {
            Output h;
            h.stdOutPipe = StdOut::pipe();
            h.stdErrPipe = StdErr::pipe();
            return h;
        }
        static PLY_INLINE Output openSeparate() {
            Output h;
            h.stdOut = Pipe::Open;
            h.stdErr = Pipe::Open;
            return h;
        }
        static PLY_INLINE Output openMerged() {
            Output h;
            h.stdOut = Pipe::Open;
            h.stdErr = Pipe::StdOut;
            return h;
        }
        static PLY_INLINE Output openStdOutOnly() {
            Output h;
            h.stdOut = Pipe::Open;
            return h;
        }
    };

    struct Input {
        Pipe stdIn = Pipe::Redirect;
        InPipe* stdInPipe = nullptr;

        static PLY_INLINE Input ignore() {
            return {};
        }
        static PLY_INLINE Input inherit() {
            return {Pipe::Redirect, StdIn::pipe()};
        }
        static PLY_INLINE Input open() {
            return {Pipe::Open, nullptr};
        }
    };

    // Members
    Owned<OutPipe> writeToStdIn;
    Owned<InPipe> readFromStdOut;
    Owned<InPipe> readFromStdErr;

    PLY_INLINE Subprocess() = default;
    virtual ~Subprocess() = default;
    virtual s32 join() = 0;

    static PLY_DLL_ENTRY Owned<Subprocess> execArgStr(const StringView exePath, const StringView argStr,
                                                      const StringView initialDir, const Output& output,
                                                      const Input& input = Input::open());
    static PLY_DLL_ENTRY Owned<Subprocess> exec(const StringView exePath,
                                                ArrayView<const StringView> args,
                                                const StringView initialDir, const Output& output,
                                                const Input& input = Input::open());
};

} // namespace ply

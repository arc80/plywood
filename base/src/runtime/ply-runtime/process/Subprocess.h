/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime.h>

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
            h.stdOutPipe = get_console_out_pipe();
            h.stdErrPipe = get_console_error_pipe();
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
            return {Pipe::Redirect, get_console_in_pipe()};
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

    static PLY_DLL_ENTRY Owned<Subprocess> execArgStr(StringView exePath, StringView argStr,
                                                      StringView initialDir, const Output& output,
                                                      const Input& input = Input::open());
    static PLY_DLL_ENTRY Owned<Subprocess> exec(StringView exePath,
                                                ArrayView<const StringView> args,
                                                StringView initialDir, const Output& output,
                                                const Input& input = Input::open());
};

} // namespace ply

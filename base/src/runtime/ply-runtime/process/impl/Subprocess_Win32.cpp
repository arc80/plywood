/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-runtime/Precomp.h>

#if PLY_TARGET_WIN32

#include <ply-runtime/process/Subprocess.h>
#include <ply-runtime/Path.h>
#include <ply-runtime/string/WString.h>

namespace ply {

struct Subprocess_Win32 : Subprocess {
    HANDLE childProcess = INVALID_HANDLE_VALUE;
    HANDLE childMainThread = INVALID_HANDLE_VALUE;

    PLY_INLINE Subprocess_Win32() = default;

    PLY_NO_INLINE ~Subprocess_Win32() {
        PLY_ASSERT(this->childProcess != INVALID_HANDLE_VALUE);
        CloseHandle(this->childProcess);
        CloseHandle(this->childMainThread);
    }

    PLY_NO_INLINE s32 join() override {
        PLY_ASSERT(this->childProcess != INVALID_HANDLE_VALUE);
        DWORD rc = WaitForSingleObject(this->childProcess, INFINITE);
        PLY_ASSERT(rc == WAIT_OBJECT_0);
        PLY_UNUSED(rc);
        // FIXME: Add an assert here to ensure that readFromStdOut & readFromStdErr have
        // been drained (?).
        DWORD exitCode;
        BOOL rc2 = GetExitCodeProcess(this->childProcess, &exitCode);
        PLY_ASSERT(rc2 != 0);
        PLY_UNUSED(rc2);
        return (s32) exitCode;
    }
};

//--------------------------------
// NULL input/output
//--------------------------------
PLY_NO_INLINE HANDLE getNullInHandle_Win32() {
    struct NullInPipe {
        HANDLE handle = INVALID_HANDLE_VALUE;

        PLY_NO_INLINE NullInPipe() {
            // FIXME: If CreateFileW fails, create a pipe here and close the sending
            // end.
            this->handle = CreateFileW(L"nul", GENERIC_READ, FILE_SHARE_READ, NULL,
                                       OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
            PLY_ASSERT(this->handle != INVALID_HANDLE_VALUE);
            BOOL rc = SetHandleInformation(this->handle, HANDLE_FLAG_INHERIT,
                                           HANDLE_FLAG_INHERIT);
            PLY_ASSERT(rc != 0);
        }

        PLY_NO_INLINE ~NullInPipe() {
            PLY_ASSERT(this->handle != INVALID_HANDLE_VALUE);
            CloseHandle(this->handle);
            // FIXME: If a thread was spawned (as per comment above), we could join it
            // here
        }
    };

    static NullInPipe nullInPipe;
    return nullInPipe.handle;
}

PLY_NO_INLINE HANDLE getNullOutHandle_Win32() {
    struct NullOutPipe {
        HANDLE handle = INVALID_HANDLE_VALUE;

        PLY_NO_INLINE NullOutPipe() {
            // FIXME: If CreateFileW fails, create a pipe here and spawn a thread that
            // infinitely consumes the pipe's output.
            this->handle = CreateFileW(L"nul", GENERIC_WRITE, FILE_SHARE_WRITE, NULL,
                                       OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
            PLY_ASSERT(this->handle != INVALID_HANDLE_VALUE);
            BOOL rc = SetHandleInformation(this->handle, HANDLE_FLAG_INHERIT,
                                           HANDLE_FLAG_INHERIT);
            PLY_ASSERT(rc != 0);
        }

        PLY_NO_INLINE ~NullOutPipe() {
            PLY_ASSERT(this->handle != INVALID_HANDLE_VALUE);
            CloseHandle(this->handle);
            // FIXME: If a thread was spawned (as per comment above), we could join it
            // here
        }
    };

    static NullOutPipe nullOutPipe;
    return nullOutPipe.handle;
}

PLY_NO_INLINE HANDLE createInheritableHandle(HANDLE origHandle) {
    HANDLE currentProcess = GetCurrentProcess();
    HANDLE dupHandle = INVALID_HANDLE_VALUE;
    BOOL rc = DuplicateHandle(currentProcess, origHandle, currentProcess, &dupHandle, 0,
                              TRUE, DUPLICATE_SAME_ACCESS);
    PLY_ASSERT(rc != 0);
    PLY_ASSERT(dupHandle != INVALID_HANDLE_VALUE);
    return dupHandle;
}

Owned<Subprocess> Subprocess::execArgStr(StringView exePath, StringView argStr,
                                         StringView initialDir, const Output& output,
                                         const Input& input) {
    // These are temporary handles meant for the subprocess to inherit.
    // They're manually closed (below) after the call to CreateProcessW:
    HANDLE hChildStd_IN_Rd = INVALID_HANDLE_VALUE;
    HANDLE hChildStd_OUT_Wr = INVALID_HANDLE_VALUE;
    HANDLE hChildStd_ERR_Wr = INVALID_HANDLE_VALUE;

    // These are the In/OutPipes that will be used to communicate with the subprocess
    // (depending on the Output settings). They'll be moved to the SubProcess object on
    // success (or automatically closed on failure):
    Owned<OutPipe> writeToChildStdIn;
    Owned<InPipe> readFromChildStdOut;
    Owned<InPipe> readFromChildStdErr;

    // Default SECURITY_ATTRIBUTES for any Win32 pipes created here:
    SECURITY_ATTRIBUTES saAttr;
    ZeroMemory(&saAttr, sizeof(saAttr));
    saAttr.nLength = sizeof(saAttr);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    // Prepare STARTUPINFO:
    STARTUPINFOW startupInfo;
    ZeroMemory(&startupInfo, sizeof(startupInfo));
    startupInfo.cb = sizeof(startupInfo);
    startupInfo.dwFlags = STARTF_USESTDHANDLES;
    startupInfo.hStdError = INVALID_HANDLE_VALUE;  // Will get filled in
    startupInfo.hStdOutput = INVALID_HANDLE_VALUE; // Will get filled in
    startupInfo.hStdInput = INVALID_HANDLE_VALUE;  // Will get filled in

    //-----------------------------------------------------------
    // Configure child process's stdin
    //-----------------------------------------------------------
    if (input.stdIn == Pipe::Redirect) {
        if (input.stdInPipe) {
            // input.stdInPipe MUST be valid InPipe_Handle. Will assert here otherwise:
            hChildStd_IN_Rd =
                createInheritableHandle(input.stdInPipe->cast<InPipe_Handle>()->handle);
            startupInfo.hStdInput = hChildStd_IN_Rd;
        } else {
            // Ignore the input
            startupInfo.hStdInput = getNullInHandle_Win32();
        }
    } else {
        PLY_ASSERT(input.stdIn == Pipe::Open);

        // Create a pipe for the child process's stdin:
        HANDLE hChildStd_IN_Wr = INVALID_HANDLE_VALUE;
        BOOL rc = CreatePipe(&hChildStd_IN_Rd, &hChildStd_IN_Wr, &saAttr, 0);
        PLY_ASSERT(rc != 0);
        PLY_ASSERT(hChildStd_IN_Rd != INVALID_HANDLE_VALUE);
        PLY_ASSERT(hChildStd_IN_Wr != INVALID_HANDLE_VALUE);
        // Ensure the write handle to the pipe for STDIN is not inherited:
        rc = SetHandleInformation(hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0);
        PLY_ASSERT(rc != 0);
        startupInfo.hStdInput = hChildStd_IN_Rd;
        writeToChildStdIn = new OutPipe_Handle{hChildStd_IN_Wr};
    }

    //-----------------------------------------------------------
    // Configure child process's stdout
    //-----------------------------------------------------------
    if (output.stdOut == Pipe::Redirect) {
        if (output.stdOutPipe) {
            // output.stdOutPipe MUST be valid OutPipe_Handle. Will assert here
            // otherwise:
            hChildStd_OUT_Wr = createInheritableHandle(
                output.stdOutPipe->cast<OutPipe_Handle>()->handle);
            startupInfo.hStdOutput = hChildStd_OUT_Wr;
        } else {
            // Ignore the output
            startupInfo.hStdOutput = getNullOutHandle_Win32();
        }
    } else {
        PLY_ASSERT(output.stdOut ==
                   Pipe::Open); // Only output.stdErr can be set to Pipe::StdOut

        // Create a pipe for the child process's stdout:
        HANDLE hChildStd_OUT_Rd = INVALID_HANDLE_VALUE;
        BOOL rc = CreatePipe(&hChildStd_OUT_Rd, &hChildStd_OUT_Wr, &saAttr, 0);
        PLY_ASSERT(rc != 0);
        PLY_ASSERT(hChildStd_OUT_Rd != INVALID_HANDLE_VALUE);
        PLY_ASSERT(hChildStd_OUT_Wr != INVALID_HANDLE_VALUE);
        // Ensure the read handle to the pipe for STDOUT is not inherited:
        rc = SetHandleInformation(hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0);
        PLY_ASSERT(rc != 0);
        startupInfo.hStdOutput = hChildStd_OUT_Wr;
        readFromChildStdOut = new InPipe_Handle{hChildStd_OUT_Rd};
    }

    //-----------------------------------------------------------
    // Configure child process's stderr
    //-----------------------------------------------------------
    if (output.stdErr == Pipe::Redirect) {
        if (output.stdErrPipe) {
            // output.stdErrPipe MUST be valid OutPipe_Handle. Will assert here
            // otherwise:
            hChildStd_ERR_Wr = createInheritableHandle(
                output.stdErrPipe->cast<OutPipe_Handle>()->handle);
            startupInfo.hStdError = hChildStd_ERR_Wr;
        } else {
            // Ignore the output
            startupInfo.hStdError = getNullOutHandle_Win32();
        }
    } else if (output.stdErr == Pipe::StdOut) {
        startupInfo.hStdError = startupInfo.hStdOutput;
    } else {
        PLY_ASSERT(output.stdErr == Pipe::Open);

        // Create a pipe for the child process's stderr:
        HANDLE hChildStd_ERR_Rd = INVALID_HANDLE_VALUE;
        BOOL rc = CreatePipe(&hChildStd_ERR_Rd, &hChildStd_ERR_Wr, &saAttr, 0);
        PLY_ASSERT(hChildStd_ERR_Rd != INVALID_HANDLE_VALUE);
        PLY_ASSERT(hChildStd_ERR_Wr != INVALID_HANDLE_VALUE);
        PLY_ASSERT(rc != 0);
        // Ensure the read handle to the pipe for STDERR is not inherited:
        rc = SetHandleInformation(hChildStd_ERR_Rd, HANDLE_FLAG_INHERIT, 0);
        PLY_ASSERT(rc != 0);
        startupInfo.hStdError = hChildStd_ERR_Wr;
        readFromChildStdErr = new InPipe_Handle{hChildStd_ERR_Rd};
    }

    // Create command line
    MemOutStream cmdLine;
    cmdLine.format(CmdLineArg_WinCrt{exePath});
    if (argStr) {
        cmdLine << ' ' << argStr;
    }
    cmdLine << '\0';
    WString wCmdLine = toWString(cmdLine.moveToString());

    // Create the child process:
    WString win32Dir;
    if (!initialDir.isEmpty()) {
        win32Dir = win32PathArg(initialDir, false);
    }
    PROCESS_INFORMATION procInfo;
    BOOL rc = CreateProcessW(NULL, wCmdLine, NULL, NULL,
                             TRUE, // inherit handles
                             0,    // creation flags
                             NULL, initialDir.isEmpty() ? NULL : (LPCWSTR) win32Dir,
                             &startupInfo, &procInfo);

    // Manually close any temporary handles that were passed to the subprocess:
    PLY_ASSERT(hChildStd_IN_Rd != INVALID_HANDLE_VALUE);
    CloseHandle(hChildStd_IN_Rd);
    if (hChildStd_OUT_Wr != INVALID_HANDLE_VALUE) {
        CloseHandle(hChildStd_OUT_Wr);
    }
    if (hChildStd_ERR_Wr != INVALID_HANDLE_VALUE) {
        CloseHandle(hChildStd_ERR_Wr);
    }

    if (!rc) {
        // Failed to create subprocess
        return nullptr;
    }

    // Success! Create Subprocess object and return it:
    Subprocess_Win32* subprocess = new Subprocess_Win32;
    PLY_ASSERT(procInfo.hProcess != INVALID_HANDLE_VALUE);
    PLY_ASSERT(procInfo.hThread != INVALID_HANDLE_VALUE);
    subprocess->childProcess = procInfo.hProcess;
    subprocess->childMainThread = procInfo.hThread;
    subprocess->writeToStdIn = std::move(writeToChildStdIn);
    subprocess->readFromStdOut = std::move(readFromChildStdOut);
    subprocess->readFromStdErr = std::move(readFromChildStdErr);
    return subprocess;
}

PLY_NO_INLINE Owned<Subprocess>
Subprocess::exec(StringView exePath, ArrayView<const StringView> args,
                 StringView initialDir, const Output& output, const Input& input) {
    MemOutStream mout;
    for (StringView arg : args) {
        if (mout.get_seek_pos() > 0) {
            mout << ' ';
        }
        mout.format(CmdLineArg_WinCrt{arg});
    }
    return Subprocess::execArgStr(exePath, mout.moveToString(), initialDir, output,
                                  input);
}

} // namespace ply

#endif // PLY_TARGET_WIN32

/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-runtime/Precomp.h>

#if PLY_TARGET_WIN32

#include <ply-runtime/string/WString.h>

namespace ply {

Process::~Process() {
    PLY_ASSERT(this->child_process != INVALID_HANDLE_VALUE);
    CloseHandle(this->child_process);
    CloseHandle(this->child_main_thread);
}

s32 Process::join() {
    PLY_ASSERT(this->child_process != INVALID_HANDLE_VALUE);
    DWORD rc = WaitForSingleObject(this->child_process, INFINITE);
    PLY_ASSERT(rc == WAIT_OBJECT_0);
    PLY_UNUSED(rc);
    // FIXME: Add an assert here to ensure that read_from_std_out & read_from_std_err
    // have been drained (?).
    DWORD exit_code;
    BOOL rc2 = GetExitCodeProcess(this->child_process, &exit_code);
    PLY_ASSERT(rc2 != 0);
    PLY_UNUSED(rc2);
    return (s32) exit_code;
}

//--------------------------------
// NULL input/output
//--------------------------------
HANDLE get_null_in_handle_win32() {
    struct NullInPipe {
        HANDLE handle = INVALID_HANDLE_VALUE;

        NullInPipe() {
            // FIXME: If CreateFileW fails, create a pipe here and close the sending
            // end.
            this->handle = CreateFileW(L"nul", GENERIC_READ, FILE_SHARE_READ, NULL,
                                       OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
            PLY_ASSERT(this->handle != INVALID_HANDLE_VALUE);
            BOOL rc = SetHandleInformation(this->handle, HANDLE_FLAG_INHERIT,
                                           HANDLE_FLAG_INHERIT);
            PLY_ASSERT(rc != 0);
        }

        ~NullInPipe() {
            PLY_ASSERT(this->handle != INVALID_HANDLE_VALUE);
            CloseHandle(this->handle);
            // FIXME: If a thread was spawned (as per comment above), we could join it
            // here
        }
    };

    static NullInPipe null_in_pipe;
    return null_in_pipe.handle;
}

HANDLE get_null_out_handle_win32() {
    struct NullOutPipe {
        HANDLE handle = INVALID_HANDLE_VALUE;

        NullOutPipe() {
            // FIXME: If CreateFileW fails, create a pipe here and spawn a thread that
            // infinitely consumes the pipe's output.
            this->handle = CreateFileW(L"nul", GENERIC_WRITE, FILE_SHARE_WRITE, NULL,
                                       OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
            PLY_ASSERT(this->handle != INVALID_HANDLE_VALUE);
            BOOL rc = SetHandleInformation(this->handle, HANDLE_FLAG_INHERIT,
                                           HANDLE_FLAG_INHERIT);
            PLY_ASSERT(rc != 0);
        }

        ~NullOutPipe() {
            PLY_ASSERT(this->handle != INVALID_HANDLE_VALUE);
            CloseHandle(this->handle);
            // FIXME: If a thread was spawned (as per comment above), we could join it
            // here
        }
    };

    static NullOutPipe null_out_pipe;
    return null_out_pipe.handle;
}

HANDLE create_inheritable_handle(HANDLE orig_handle) {
    HANDLE current_process = GetCurrentProcess();
    HANDLE dup_handle = INVALID_HANDLE_VALUE;
    BOOL rc = DuplicateHandle(current_process, orig_handle, current_process,
                              &dup_handle, 0, TRUE, DUPLICATE_SAME_ACCESS);
    PLY_ASSERT(rc != 0);
    PLY_ASSERT(dup_handle != INVALID_HANDLE_VALUE);
    return dup_handle;
}

Owned<Process> Process::exec_arg_str(StringView exe_path, StringView arg_str,
                                     StringView initial_dir, const Output& output,
                                     const Input& input) {
    // These are temporary handles meant for the subprocess to inherit.
    // They're manually closed (below) after the call to CreateProcessW:
    HANDLE h_child_stdin_rd = INVALID_HANDLE_VALUE;
    HANDLE h_child_stdout_wr = INVALID_HANDLE_VALUE;
    HANDLE h_child_stderr_wr = INVALID_HANDLE_VALUE;

    // These are the In/OutPipes that will be used to communicate with the subprocess
    // (depending on the Output settings). They'll be moved to the SubProcess object on
    // success (or automatically closed on failure):
    Owned<OutPipe> write_to_child_std_in;
    Owned<InPipe> read_from_child_std_out;
    Owned<InPipe> read_from_child_std_err;

    // Default SECURITY_ATTRIBUTES for any Win32 pipes created here:
    SECURITY_ATTRIBUTES sa_attr;
    ZeroMemory(&sa_attr, sizeof(sa_attr));
    sa_attr.nLength = sizeof(sa_attr);
    sa_attr.bInheritHandle = TRUE;
    sa_attr.lpSecurityDescriptor = NULL;

    // Prepare STARTUPINFO:
    STARTUPINFOW startup_info;
    ZeroMemory(&startup_info, sizeof(startup_info));
    startup_info.cb = sizeof(startup_info);
    startup_info.dwFlags = STARTF_USESTDHANDLES;
    startup_info.hStdError = INVALID_HANDLE_VALUE;  // Will get filled in
    startup_info.hStdOutput = INVALID_HANDLE_VALUE; // Will get filled in
    startup_info.hStdInput = INVALID_HANDLE_VALUE;  // Will get filled in

    //-----------------------------------------------------------
    // Configure child process's stdin
    //-----------------------------------------------------------
    if (input.std_in == Pipe::Redirect) {
        if (input.std_in_pipe) {
            // input.std_in_pipe MUST be valid InPipe_Handle. Will assert here
            // otherwise:
            h_child_stdin_rd = create_inheritable_handle(
                input.std_in_pipe->cast<InPipe_Handle>()->handle);
            startup_info.hStdInput = h_child_stdin_rd;
        } else {
            // Ignore the input
            startup_info.hStdInput = get_null_in_handle_win32();
        }
    } else {
        PLY_ASSERT(input.std_in == Pipe::Open);

        // Create a pipe for the child process's stdin:
        HANDLE h_child_stdin_wr = INVALID_HANDLE_VALUE;
        BOOL rc = CreatePipe(&h_child_stdin_rd, &h_child_stdin_wr, &sa_attr, 0);
        PLY_ASSERT(rc != 0);
        PLY_ASSERT(h_child_stdin_rd != INVALID_HANDLE_VALUE);
        PLY_ASSERT(h_child_stdin_wr != INVALID_HANDLE_VALUE);
        // Ensure the write handle to the pipe for STDIN is not inherited:
        rc = SetHandleInformation(h_child_stdin_wr, HANDLE_FLAG_INHERIT, 0);
        PLY_ASSERT(rc != 0);
        startup_info.hStdInput = h_child_stdin_rd;
        write_to_child_std_in = new OutPipe_Handle{h_child_stdin_wr};
    }

    //-----------------------------------------------------------
    // Configure child process's stdout
    //-----------------------------------------------------------
    if (output.std_out == Pipe::Redirect) {
        if (output.std_out_pipe) {
            // output.std_out_pipe MUST be valid OutPipe_Handle. Will assert here
            // otherwise:
            h_child_stdout_wr = create_inheritable_handle(
                output.std_out_pipe->cast<OutPipe_Handle>()->handle);
            startup_info.hStdOutput = h_child_stdout_wr;
        } else {
            // Ignore the output
            startup_info.hStdOutput = get_null_out_handle_win32();
        }
    } else {
        PLY_ASSERT(output.std_out ==
                   Pipe::Open); // Only output.std_err can be set to Pipe::StdOut

        // Create a pipe for the child process's stdout:
        HANDLE h_child_stdout_rd = INVALID_HANDLE_VALUE;
        BOOL rc = CreatePipe(&h_child_stdout_rd, &h_child_stdout_wr, &sa_attr, 0);
        PLY_ASSERT(rc != 0);
        PLY_ASSERT(h_child_stdout_rd != INVALID_HANDLE_VALUE);
        PLY_ASSERT(h_child_stdout_wr != INVALID_HANDLE_VALUE);
        // Ensure the read handle to the pipe for STDOUT is not inherited:
        rc = SetHandleInformation(h_child_stdout_rd, HANDLE_FLAG_INHERIT, 0);
        PLY_ASSERT(rc != 0);
        startup_info.hStdOutput = h_child_stdout_wr;
        read_from_child_std_out = new InPipe_Handle{h_child_stdout_rd};
    }

    //-----------------------------------------------------------
    // Configure child process's stderr
    //-----------------------------------------------------------
    if (output.std_err == Pipe::Redirect) {
        if (output.std_err_pipe) {
            // output.std_err_pipe MUST be valid OutPipe_Handle. Will assert here
            // otherwise:
            h_child_stderr_wr = create_inheritable_handle(
                output.std_err_pipe->cast<OutPipe_Handle>()->handle);
            startup_info.hStdError = h_child_stderr_wr;
        } else {
            // Ignore the output
            startup_info.hStdError = get_null_out_handle_win32();
        }
    } else if (output.std_err == Pipe::StdOut) {
        startup_info.hStdError = startup_info.hStdOutput;
    } else {
        PLY_ASSERT(output.std_err == Pipe::Open);

        // Create a pipe for the child process's stderr:
        HANDLE h_child_stderr_rd = INVALID_HANDLE_VALUE;
        BOOL rc = CreatePipe(&h_child_stderr_rd, &h_child_stderr_wr, &sa_attr, 0);
        PLY_ASSERT(h_child_stderr_rd != INVALID_HANDLE_VALUE);
        PLY_ASSERT(h_child_stderr_wr != INVALID_HANDLE_VALUE);
        PLY_ASSERT(rc != 0);
        // Ensure the read handle to the pipe for STDERR is not inherited:
        rc = SetHandleInformation(h_child_stderr_rd, HANDLE_FLAG_INHERIT, 0);
        PLY_ASSERT(rc != 0);
        startup_info.hStdError = h_child_stderr_wr;
        read_from_child_std_err = new InPipe_Handle{h_child_stderr_rd};
    }

    // Create command line
    MemOutStream cmd_line;
    cmd_line.format(CmdLineArg_WinCrt{exe_path});
    if (arg_str) {
        cmd_line << ' ' << arg_str;
    }
    cmd_line << '\0';
    WString w_cmd_line = to_wstring(cmd_line.move_to_string());

    // Create the child process:
    WString win32_dir;
    if (!initial_dir.is_empty()) {
        win32_dir = win32_path_arg(initial_dir, false);
    }
    PROCESS_INFORMATION proc_info;
    BOOL rc = CreateProcessW(NULL, w_cmd_line, NULL, NULL,
                             TRUE, // inherit handles
                             0,    // creation flags
                             NULL, initial_dir.is_empty() ? NULL : (LPCWSTR) win32_dir,
                             &startup_info, &proc_info);

    // Manually close any temporary handles that were passed to the subprocess:
    PLY_ASSERT(h_child_stdin_rd != INVALID_HANDLE_VALUE);
    CloseHandle(h_child_stdin_rd);
    if (h_child_stdout_wr != INVALID_HANDLE_VALUE) {
        CloseHandle(h_child_stdout_wr);
    }
    if (h_child_stderr_wr != INVALID_HANDLE_VALUE) {
        CloseHandle(h_child_stderr_wr);
    }

    if (!rc) {
        // Failed to create subprocess
        return nullptr;
    }

    // Success! Create Process object and return it:
    Process* subprocess = new Process;
    PLY_ASSERT(proc_info.hProcess != INVALID_HANDLE_VALUE);
    PLY_ASSERT(proc_info.hThread != INVALID_HANDLE_VALUE);
    subprocess->child_process = proc_info.hProcess;
    subprocess->child_main_thread = proc_info.hThread;
    subprocess->write_to_std_in = std::move(write_to_child_std_in);
    subprocess->read_from_std_out = std::move(read_from_child_std_out);
    subprocess->read_from_std_err = std::move(read_from_child_std_err);
    return subprocess;
}

Owned<Process> Process::exec(StringView exe_path, ArrayView<const StringView> args,
                             StringView initial_dir, const Output& output,
                             const Input& input) {
    MemOutStream mout;
    for (StringView arg : args) {
        if (mout.get_seek_pos() > 0) {
            mout << ' ';
        }
        mout.format(CmdLineArg_WinCrt{arg});
    }
    return Process::exec_arg_str(exe_path, mout.move_to_string(), initial_dir, output,
                                 input);
}

} // namespace ply

#endif // PLY_TARGET_WIN32

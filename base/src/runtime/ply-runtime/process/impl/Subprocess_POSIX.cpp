/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-runtime/Precomp.h>

#if PLY_TARGET_POSIX

#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

namespace ply {

Process::~Process() {
    if (this->child_pid != -1) {
        // Do a non-blocking (WNOHANG) wait so that the kernel is free to re-use the
        // child PID for other processes. In other words, avoid leaving child processes
        // in a "zombie" state.
        int status;
        int rc = waitpid(this->child_pid, &status, WNOHANG); // non-blocking
        PLY_ASSERT(rc == this->child_pid || rc == 0);
        PLY_UNUSED(rc);
    }
}

s32 Process::join() {
    PLY_ASSERT(this->child_pid != -1);
    int status;
    int rc;
    // Loop to ignore signals sent by the debugger on mac_os
    do {
        rc = waitpid(this->child_pid, &status, 0);
    } while (rc == -1 && errno == EINTR);
    PLY_ASSERT(rc == this->child_pid);
    PLY_UNUSED(rc);
    this->child_pid = -1;
    // FIXME: Add an assert here to ensure that read_from_std_out & read_from_std_err
    // have been drained (?).
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    } else {
        return -1;
    }
}

//--------------------------------
// Get unique inheritable handle for null output
//--------------------------------
int get_null_in_fd_posix() {
    static int fd = -1;
    if (fd == -1) {
        static Mutex mutex;
        LockGuard<Mutex> guard{mutex};
        if (fd == -1) {
            // FIXME: If open() fails, create a pipe here and spawn a thread that
            // infinitely consumes the pipe's output.
            fd = open("/dev/null", O_RDONLY);
            PLY_ASSERT(fd != -1);
        }
    }
    int fd2 = dup(fd);
    PLY_ASSERT(fd2 != -1);
    return fd2;
}

int get_null_out_fd_posix() {
    static int fd = -1;
    if (fd == -1) {
        static Mutex mutex;
        LockGuard<Mutex> guard{mutex};
        if (fd == -1) {
            // FIXME: If open() fails, create a pipe here and spawn a thread that
            // infinitely consumes the pipe's output.
            fd = open("/dev/null", O_WRONLY);
            PLY_ASSERT(fd != -1);
        }
    }
    int fd2 = dup(fd);
    PLY_ASSERT(fd2 != -1);
    return fd2;
}

Owned<Process> Process::exec(StringView exe_path, ArrayView<const StringView> args,
                             StringView initial_dir, const Output& output,
                             const Input& input) {
    int child_std_in_fd[2] = {-1, -1};  // 0 is read, 1 is write
    int child_std_out_fd[2] = {-1, -1}; // 0 is read, 1 is write
    int child_std_err_fd[2] = {-1, -1}; // 0 is read, 1 is write

    //-----------------------------------------------------------
    // Configure child process's stdin
    //-----------------------------------------------------------
    if (input.std_in == Pipe::Redirect) {
        if (input.std_in_pipe) {
            // output.std_out_pipe MUST be valid OutPipe_FD. Will assert here otherwise:
            child_std_in_fd[0] = dup(input.std_in_pipe->cast<InPipe_FD>()->fd);
            PLY_ASSERT(child_std_in_fd[0] != -1);
        } else {
            // ignore the input
            child_std_in_fd[0] = get_null_in_fd_posix();
            PLY_ASSERT(child_std_in_fd[0] != -1);
        }
    } else {
        PLY_ASSERT(input.std_in == Pipe::Open);

        // Create a pipe for the child process's stdin:
        int rc = pipe(child_std_in_fd);
        PLY_ASSERT(rc == 0);
        PLY_UNUSED(rc);
        // Ensure the write handle to the pipe for STDIN is not inherited:
        rc = fcntl(child_std_in_fd[1], F_SETFD, FD_CLOEXEC);
        PLY_ASSERT(rc == 0);
    }

    //-----------------------------------------------------------
    // Configure child process's stdout
    //-----------------------------------------------------------
    if (output.std_out == Pipe::Redirect) {
        if (output.std_out_pipe) {
            // output.std_out_pipe MUST be valid OutPipe_FD. Will assert here otherwise:
            child_std_out_fd[1] = dup(output.std_out_pipe->cast<OutPipe_FD>()->fd);
            PLY_ASSERT(child_std_out_fd[1] != -1);
        } else {
            // ignore the output
            child_std_out_fd[1] = get_null_out_fd_posix();
            PLY_ASSERT(child_std_out_fd[1] != -1);
        }
    } else {
        PLY_ASSERT(output.std_out ==
                   Pipe::Open); // Only output.std_err can be set to Pipe::StdOut

        // Create a pipe for the child process's stdout:
        int rc = pipe(child_std_out_fd);
        PLY_ASSERT(rc == 0);
        PLY_UNUSED(rc);
        // Ensure the read handle to the pipe for STDOUT is not inherited:
        rc = fcntl(child_std_out_fd[0], F_SETFD, FD_CLOEXEC);
        PLY_ASSERT(rc == 0);
    }

    //-----------------------------------------------------------
    // Configure child process's stderr
    //-----------------------------------------------------------
    if (output.std_err == Pipe::Redirect) {
        if (output.std_err_pipe) {
            // output.std_err_pipe MUST be valid OutPipe_FD. Will assert here otherwise:
            child_std_err_fd[1] = dup(output.std_err_pipe->cast<OutPipe_FD>()->fd);
            PLY_ASSERT(child_std_err_fd[1] != -1);
        } else {
            // ignore the output
            child_std_err_fd[1] = get_null_out_fd_posix();
            PLY_ASSERT(child_std_err_fd[1] != -1);
        }
    } else if (output.std_err == Pipe::StdOut) {
        PLY_ASSERT(child_std_out_fd[1] != -1);
        child_std_err_fd[1] = child_std_out_fd[1];
    } else {
        PLY_ASSERT(output.std_err == Pipe::Open);

        // Create a pipe for the child process's stderr:
        int rc = pipe(child_std_err_fd);
        PLY_ASSERT(rc == 0);
        PLY_UNUSED(rc);
        // Ensure the read handle to the pipe for STDERR is not inherited:
        rc = fcntl(child_std_err_fd[0], F_SETFD, FD_CLOEXEC);
        PLY_ASSERT(rc == 0);
    }

    //-----------------------------------------------------------
    // Prepare args to execv
    //-----------------------------------------------------------
    // FIXME: Optimize for fewer memory allocations
    String null_terminated_exe_path = exe_path.with_null_terminator();
    String null_terminated_initial_dir;
    if (initial_dir) {
        null_terminated_initial_dir = initial_dir.with_null_terminator();
    }
    Array<String> null_terminated_args;
    Array<char*> args_to_execv;
    null_terminated_args.resize(args.num_items);
    args_to_execv.resize(args.num_items + 2);
    args_to_execv[0] = null_terminated_exe_path.bytes;
    for (u32 i = 0; i < args.num_items; i++) {
        null_terminated_args[i] = args[i].with_null_terminator();
        args_to_execv[i + 1] = null_terminated_args[i].bytes;
    }
    args_to_execv[args.num_items + 1] = nullptr;

    //-----------------------------------------------------------
    // Fork the process
    //-----------------------------------------------------------
    int child_pid = fork();
    PLY_ASSERT(child_pid >= 0);
    if (child_pid == 0) {
        // We're in the child process.
        // Redirect stdin and close temporary file descriptors:
        int rc = dup2(child_std_in_fd[0], STDIN_FILENO);
        PLY_ASSERT(rc == STDIN_FILENO);
        rc = close(child_std_in_fd[0]);
        PLY_ASSERT(rc == 0);

        // Redirect stdout and close temporary file descriptors:
        rc = dup2(child_std_out_fd[1], STDOUT_FILENO);
        PLY_ASSERT(rc == STDOUT_FILENO);
        if (child_std_out_fd[1] != child_std_err_fd[1]) {
            rc = close(child_std_out_fd[1]);
            PLY_ASSERT(rc == 0);
        }

        // Redirect stderr and close temporary file descriptors:
        rc = dup2(child_std_err_fd[1], STDERR_FILENO);
        PLY_ASSERT(rc == STDERR_FILENO);
        rc = close(child_std_err_fd[1]);
        PLY_ASSERT(rc == 0);

        // Exec the new process:
        if (null_terminated_initial_dir) {
            rc = chdir(null_terminated_initial_dir.bytes);
        }
        rc = execvp(null_terminated_exe_path.bytes, args_to_execv.get());
        PLY_UNUSED(rc);
        abort(); // abort if there's an error
    }

    // This is the parent process.
    // Create Process object and return it:
    Process* subprocess = new Process;
    subprocess->child_pid = child_pid;
    int rc = close(child_std_in_fd[0]);
    PLY_ASSERT(rc == 0);
    if (child_std_in_fd[1] >= 0) {
        subprocess->write_to_std_in = new OutPipe_FD{child_std_in_fd[1]};
    }
    rc = close(child_std_out_fd[1]);
    PLY_ASSERT(rc == 0);
    if (child_std_out_fd[0] >= 0) {
        subprocess->read_from_std_out = new InPipe_FD{child_std_out_fd[0]};
    }
    if (child_std_out_fd[1] != child_std_err_fd[1]) {
        rc = close(child_std_err_fd[1]);
    }
    PLY_ASSERT(rc == 0);
    PLY_UNUSED(rc);
    if (child_std_err_fd[0] >= 0) {
        subprocess->read_from_std_err = new InPipe_FD{child_std_err_fd[0]};
    }
    return subprocess;
}

} // namespace ply

#endif // PLY_TARGET_POSIX

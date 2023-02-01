/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-runtime/Precomp.h>

#if PLY_TARGET_POSIX

#include <ply-runtime/process/Subprocess.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

namespace ply {

struct Subprocess_POSIX : Subprocess {
    int childPID = -1;

    PLY_INLINE Subprocess_POSIX() = default;

    PLY_NO_INLINE ~Subprocess_POSIX() {
        if (this->childPID != -1) {
            // Do a non-blocking (WNOHANG) wait so that the kernel is free to re-use the child PID
            // for other processes. In other words, avoid leaving child processes in a "zombie"
            // state.
            int status;
            int rc = waitpid(this->childPID, &status, WNOHANG); // non-blocking
            PLY_ASSERT(rc == this->childPID || rc == 0);
            PLY_UNUSED(rc);
        }
    }

    PLY_NO_INLINE s32 join() override {
        PLY_ASSERT(this->childPID != -1);
        int status;
        int rc;
        // Loop to ignore signals sent by the debugger on macOS
        do {
            rc = waitpid(this->childPID, &status, 0);
        } while (rc == -1 && errno == EINTR);
        PLY_ASSERT(rc == this->childPID);
        PLY_UNUSED(rc);
        this->childPID = -1;
        // FIXME: Add an assert here to ensure that readFromStdOut & readFromStdErr have been
        // drained (?).
        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        } else {
            return -1;
        }
    }
};

//--------------------------------
// Get unique inheritable handle for null output
//--------------------------------
PLY_NO_INLINE int getNullInFD_POSIX() {
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

PLY_NO_INLINE int getNullOutFD_POSIX() {
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

PLY_NO_INLINE Owned<Subprocess> Subprocess::exec(StringView exePath,
                                                 ArrayView<const StringView> args,
                                                 StringView initialDir, const Output& output,
                                                 const Input& input) {
    int childStdInFD[2] = {-1, -1};  // 0 is read, 1 is write
    int childStdOutFD[2] = {-1, -1}; // 0 is read, 1 is write
    int childStdErrFD[2] = {-1, -1}; // 0 is read, 1 is write

    //-----------------------------------------------------------
    // Configure child process's stdin
    //-----------------------------------------------------------
    if (input.stdIn == Pipe::Redirect) {
        if (input.stdInPipe) {
            // output.stdOutPipe MUST be valid OutPipe_FD. Will assert here otherwise:
            childStdInFD[0] = dup(input.stdInPipe->cast<InPipe_FD>()->fd);
            PLY_ASSERT(childStdInFD[0] != -1);
        } else {
            // ignore the input
            childStdInFD[0] = getNullInFD_POSIX();
            PLY_ASSERT(childStdInFD[0] != -1);
        }
    } else {
        PLY_ASSERT(input.stdIn == Pipe::Open);

        // Create a pipe for the child process's stdin:
        int rc = pipe(childStdInFD);
        PLY_ASSERT(rc == 0);
        PLY_UNUSED(rc);
        // Ensure the write handle to the pipe for STDIN is not inherited:
        rc = fcntl(childStdInFD[1], F_SETFD, FD_CLOEXEC);
        PLY_ASSERT(rc == 0);
    }

    //-----------------------------------------------------------
    // Configure child process's stdout
    //-----------------------------------------------------------
    if (output.stdOut == Pipe::Redirect) {
        if (output.stdOutPipe) {
            // output.stdOutPipe MUST be valid OutPipe_FD. Will assert here otherwise:
            childStdOutFD[1] = dup(output.stdOutPipe->cast<OutPipe_FD>()->fd);
            PLY_ASSERT(childStdOutFD[1] != -1);
        } else {
            // ignore the output
            childStdOutFD[1] = getNullOutFD_POSIX();
            PLY_ASSERT(childStdOutFD[1] != -1);
        }
    } else {
        PLY_ASSERT(output.stdOut == Pipe::Open); // Only output.stdErr can be set to Pipe::StdOut

        // Create a pipe for the child process's stdout:
        int rc = pipe(childStdOutFD);
        PLY_ASSERT(rc == 0);
        PLY_UNUSED(rc);
        // Ensure the read handle to the pipe for STDOUT is not inherited:
        rc = fcntl(childStdOutFD[0], F_SETFD, FD_CLOEXEC);
        PLY_ASSERT(rc == 0);
    }

    //-----------------------------------------------------------
    // Configure child process's stderr
    //-----------------------------------------------------------
    if (output.stdErr == Pipe::Redirect) {
        if (output.stdErrPipe) {
            // output.stdErrPipe MUST be valid OutPipe_FD. Will assert here otherwise:
            childStdErrFD[1] = dup(output.stdErrPipe->cast<OutPipe_FD>()->fd);
            PLY_ASSERT(childStdErrFD[1] != -1);
        } else {
            // ignore the output
            childStdErrFD[1] = getNullOutFD_POSIX();
            PLY_ASSERT(childStdErrFD[1] != -1);
        }
    } else if (output.stdErr == Pipe::StdOut) {
        PLY_ASSERT(childStdOutFD[1] != -1);
        childStdErrFD[1] = childStdOutFD[1];
    } else {
        PLY_ASSERT(output.stdErr == Pipe::Open);

        // Create a pipe for the child process's stderr:
        int rc = pipe(childStdErrFD);
        PLY_ASSERT(rc == 0);
        PLY_UNUSED(rc);
        // Ensure the read handle to the pipe for STDERR is not inherited:
        rc = fcntl(childStdErrFD[0], F_SETFD, FD_CLOEXEC);
        PLY_ASSERT(rc == 0);
    }

    //-----------------------------------------------------------
    // Prepare args to execv
    //-----------------------------------------------------------
    // FIXME: Optimize for fewer memory allocations
    String nullTerminatedExePath = exePath.withNullTerminator();
    String nullTerminatedInitialDir;
    if (initialDir) {
        nullTerminatedInitialDir = initialDir.withNullTerminator();
    }
    Array<String> nullTerminatedArgs;
    Array<char*> argsToExecv;
    nullTerminatedArgs.resize(args.numItems);
    argsToExecv.resize(args.numItems + 2);
    argsToExecv[0] = nullTerminatedExePath.bytes;
    for (u32 i = 0; i < args.numItems; i++) {
        nullTerminatedArgs[i] = args[i].withNullTerminator();
        argsToExecv[i + 1] = nullTerminatedArgs[i].bytes;
    }
    argsToExecv[args.numItems + 1] = nullptr;

    //-----------------------------------------------------------
    // Fork the process
    //-----------------------------------------------------------
    int childPID = fork();
    PLY_ASSERT(childPID >= 0);
    if (childPID == 0) {
        // We're in the child process.
        // Redirect stdin and close temporary file descriptors:
        int rc = dup2(childStdInFD[0], STDIN_FILENO);
        PLY_ASSERT(rc == STDIN_FILENO);
        rc = close(childStdInFD[0]);
        PLY_ASSERT(rc == 0);

        // Redirect stdout and close temporary file descriptors:
        rc = dup2(childStdOutFD[1], STDOUT_FILENO);
        PLY_ASSERT(rc == STDOUT_FILENO);
        if (childStdOutFD[1] != childStdErrFD[1]) {
            rc = close(childStdOutFD[1]);
            PLY_ASSERT(rc == 0);
        }

        // Redirect stderr and close temporary file descriptors:
        rc = dup2(childStdErrFD[1], STDERR_FILENO);
        PLY_ASSERT(rc == STDERR_FILENO);
        rc = close(childStdErrFD[1]);
        PLY_ASSERT(rc == 0);

        // Exec the new process:
        if (nullTerminatedInitialDir) {
            rc = chdir(nullTerminatedInitialDir.bytes);
        }
        rc = execvp(nullTerminatedExePath.bytes, argsToExecv.get());
        PLY_UNUSED(rc);
        abort(); // abort if there's an error
    }

    // This is the parent process.
    // Create Subprocess object and return it:
    Subprocess_POSIX* subprocess = new Subprocess_POSIX;
    subprocess->childPID = childPID;
    int rc = close(childStdInFD[0]);
    PLY_ASSERT(rc == 0);
    if (childStdInFD[1] >= 0) {
        subprocess->writeToStdIn = new OutPipe_FD{childStdInFD[1]};
    }
    rc = close(childStdOutFD[1]);
    PLY_ASSERT(rc == 0);
    if (childStdOutFD[0] >= 0) {
        subprocess->readFromStdOut = new InPipe_FD{childStdOutFD[0]};
    }
    if (childStdOutFD[1] != childStdErrFD[1]) {
        rc = close(childStdErrFD[1]);
    }
    PLY_ASSERT(rc == 0);
    PLY_UNUSED(rc);
    if (childStdErrFD[0] >= 0) {
        subprocess->readFromStdErr = new InPipe_FD{childStdErrFD[0]};
    }
    return subprocess;
}

} // namespace ply

#endif // PLY_TARGET_POSIX

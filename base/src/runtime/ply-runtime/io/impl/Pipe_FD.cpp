/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-runtime/Precomp.h>

#if PLY_TARGET_POSIX

namespace ply {

PLY_NO_INLINE void InPipe_FD_destroy(InPipe* in_pipe_) {
    InPipe_FD* in_pipe = static_cast<InPipe_FD*>(in_pipe_);
    if (in_pipe->fd >= 0) {
        int rc = ::close(in_pipe->fd);
        PLY_ASSERT(rc == 0);
        PLY_UNUSED(rc);
    }
}

PLY_NO_INLINE u32 InPipe_FD_readSome(InPipe* in_pipe_, MutStringView buf) {
    InPipe_FD* in_pipe = static_cast<InPipe_FD*>(in_pipe_);
    PLY_ASSERT(in_pipe->fd >= 0);
    // Retry as long as read() keeps failing due to EINTR caused by the debugger:
    s32 rc;
    do {
        rc = (s32)::read(in_pipe->fd, buf.bytes, buf.num_bytes);
    } while (rc == -1 && errno == EINTR);
    PLY_ASSERT(rc >= 0); // Note: Will probably need to detect closed pipes here
    if (rc < 0)
        return 0;
    return rc;
}

PLY_NO_INLINE u64 InPipe_FD_getFileSize(const InPipe* in_pipe_) {
    const InPipe_FD* in_pipe = static_cast<const InPipe_FD*>(in_pipe_);
    PLY_ASSERT(in_pipe->fd >= 0);
    struct stat buf;
    int rc = fstat(in_pipe->fd, &buf);
    PLY_ASSERT(rc == 0);
    PLY_UNUSED(rc);
    return buf.st_size;
}

InPipe::Funcs InPipe_FD::Funcs_ = {
    InPipe_FD_destroy,
    InPipe_FD_readSome,
    InPipe_FD_getFileSize,
};

PLY_NO_INLINE InPipe_FD::InPipe_FD(int fd) : InPipe{&Funcs_}, fd{fd} {
}

PLY_NO_INLINE void OutPipe_FD_destroy(OutPipe* out_pipe_) {
    OutPipe_FD* out_pipe = static_cast<OutPipe_FD*>(out_pipe_);
    if (out_pipe->fd >= 0) {
        int rc = ::close(out_pipe->fd);
        PLY_ASSERT(rc == 0);
        PLY_UNUSED(rc);
    }
}

PLY_NO_INLINE bool OutPipe_FD_write(OutPipe* out_pipe_, StringView buf) {
    OutPipe_FD* out_pipe = static_cast<OutPipe_FD*>(out_pipe_);
    PLY_ASSERT(out_pipe->fd >= 0);
    while (buf.num_bytes > 0) {
        s32 sent = (s32)::write(out_pipe->fd, buf.bytes, buf.num_bytes);
        if (sent <= 0)
            return false;
        PLY_ASSERT((u32) sent <= buf.num_bytes);
        buf.bytes += sent;
        buf.num_bytes -= sent;
    }
    return true;
}

PLY_NO_INLINE bool OutPipe_FD_flush(OutPipe* out_pipe_, bool to_device) {
    // FIXME: Implement as per
    // https://github.com/libuv/libuv/issues/1579#issue-262113760
    return true;
}

PLY_NO_INLINE u64 OutPipe_FD_seek(OutPipe* out_pipe_, s64 pos, SeekDir seek_dir) {
    OutPipe_FD* out_pipe = static_cast<OutPipe_FD*>(out_pipe_);
    PLY_ASSERT(out_pipe->fd >= 0);
    int whence;
    switch (seek_dir) {
        case SeekDir::Set:
        default:
            whence = SEEK_SET;
            break;
        case SeekDir::Cur:
            whence = SEEK_CUR;
            break;
        case SeekDir::End:
            whence = SEEK_END;
            break;
    }
    off_t rc = lseek(out_pipe->fd, check_cast<off_t>(pos), whence);
    if (rc < 0) {
        PLY_ASSERT(0); // Need to recognize error codes here
        return 0;
    }
    return rc;
}

OutPipe::Funcs OutPipe_FD::Funcs_ = {
    OutPipe_FD_destroy,
    OutPipe_FD_write,
    OutPipe_FD_flush,
    OutPipe_FD_seek,
};

PLY_NO_INLINE OutPipe_FD::OutPipe_FD(int fd) : OutPipe{&Funcs_}, fd{fd} {
}

} // namespace ply

#endif // PLY_TARGET_POSIX

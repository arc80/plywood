/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-runtime/Precomp.h>

#if PLY_TARGET_POSIX

namespace ply {

PLY_NO_INLINE void InPipe_FD_destroy(InPipe* inPipe_) {
    InPipe_FD* inPipe = static_cast<InPipe_FD*>(inPipe_);
    if (inPipe->fd >= 0) {
        int rc = ::close(inPipe->fd);
        PLY_ASSERT(rc == 0);
        PLY_UNUSED(rc);
    }
}

PLY_NO_INLINE u32 InPipe_FD_readSome(InPipe* inPipe_, MutStringView buf) {
    InPipe_FD* inPipe = static_cast<InPipe_FD*>(inPipe_);
    PLY_ASSERT(inPipe->fd >= 0);
    // Retry as long as read() keeps failing due to EINTR caused by the debugger:
    s32 rc;
    do {
        rc = (s32)::read(inPipe->fd, buf.bytes, buf.numBytes);
    } while (rc == -1 && errno == EINTR);
    PLY_ASSERT(rc >= 0); // Note: Will probably need to detect closed pipes here
    if (rc < 0)
        return 0;
    return rc;
}

PLY_NO_INLINE u64 InPipe_FD_getFileSize(const InPipe* inPipe_) {
    const InPipe_FD* inPipe = static_cast<const InPipe_FD*>(inPipe_);
    PLY_ASSERT(inPipe->fd >= 0);
    struct stat buf;
    int rc = fstat(inPipe->fd, &buf);
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

PLY_NO_INLINE void OutPipe_FD_destroy(OutPipe* outPipe_) {
    OutPipe_FD* outPipe = static_cast<OutPipe_FD*>(outPipe_);
    if (outPipe->fd >= 0) {
        int rc = ::close(outPipe->fd);
        PLY_ASSERT(rc == 0);
        PLY_UNUSED(rc);
    }
}

PLY_NO_INLINE bool OutPipe_FD_write(OutPipe* outPipe_, StringView buf) {
    OutPipe_FD* outPipe = static_cast<OutPipe_FD*>(outPipe_);
    PLY_ASSERT(outPipe->fd >= 0);
    while (buf.numBytes > 0) {
        s32 sent = (s32)::write(outPipe->fd, buf.bytes, buf.numBytes);
        if (sent <= 0)
            return false;
        PLY_ASSERT((u32) sent <= buf.numBytes);
        buf.bytes += sent;
        buf.numBytes -= sent;
    }
    return true;
}

PLY_NO_INLINE bool OutPipe_FD_flush(OutPipe* outPipe_, bool toDevice) {
    // FIXME: Implement as per
    // https://github.com/libuv/libuv/issues/1579#issue-262113760
    return true;
}

PLY_NO_INLINE u64 OutPipe_FD_seek(OutPipe* outPipe_, s64 pos, SeekDir seekDir) {
    OutPipe_FD* outPipe = static_cast<OutPipe_FD*>(outPipe_);
    PLY_ASSERT(outPipe->fd >= 0);
    int whence;
    switch (seekDir) {
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
    off_t rc = lseek(outPipe->fd, safeDemote<off_t>(pos), whence);
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

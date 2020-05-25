/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-runtime/Precomp.h>

#if PLY_TARGET_WIN32

#include <ply-runtime/io/impl/Pipe_Winsock.h>

namespace ply {

PLY_NO_INLINE void InPipe_Winsock_destroy(InPipe* inPipe_) {
    InPipe_Winsock* inPipe = static_cast<InPipe_Winsock*>(inPipe_);
    if (inPipe->socket != INVALID_SOCKET) {
        int rc = closesocket(inPipe->socket);
        PLY_ASSERT(rc == 0);
        PLY_UNUSED(rc);
    }
}

PLY_NO_INLINE u32 InPipe_Winsock_readSome(InPipe* inPipe_, BufferView buf) {
    InPipe_Winsock* inPipe = static_cast<InPipe_Winsock*>(inPipe_);
    int rc = recv(inPipe->socket, (char*) buf.bytes, int(buf.numBytes), 0);
    if (rc == 0 || rc == SOCKET_ERROR)
        return 0;
    PLY_ASSERT(rc > 0);
    return rc;
}

InPipe::Funcs InPipe_Winsock::Funcs_ = {
    InPipe_Winsock_destroy,
    InPipe_Winsock_readSome,
    InPipe::getFileSize_Unsupported,
};

PLY_NO_INLINE InPipe_Winsock::InPipe_Winsock(SOCKET socket) : InPipe{&Funcs_}, socket{socket} {
}

PLY_NO_INLINE void OutPipe_Winsock_destroy(OutPipe* outPipe_) {
    OutPipe_Winsock* outPipe = static_cast<OutPipe_Winsock*>(outPipe_);
    if (outPipe->socket != INVALID_SOCKET) {
        int rc = closesocket(outPipe->socket);
        PLY_ASSERT(rc == 0);
        PLY_UNUSED(rc);
    }
}

PLY_NO_INLINE bool OutPipe_Winsock_write(OutPipe* outPipe_, ConstBufferView buf) {
    OutPipe_Winsock* outPipe = static_cast<OutPipe_Winsock*>(outPipe_);
    while (buf.numBytes > 0) {
        int rc = send(outPipe->socket, (const char*) buf.bytes, (DWORD) buf.numBytes, 0);
        if (rc == SOCKET_ERROR) // FIXME: Test to make sure that disconnected sockets return
                                // SOCKET_ERROR and not 0
            return false;
        PLY_ASSERT(rc >= 0 && u32(rc) <= buf.numBytes);
        buf.offsetHead(rc);
    }
    return true;
}

PLY_NO_INLINE bool OutPipe_Winsock_flush(OutPipe* outPipe_, bool toDevice) {
    bool result = true;
    if (toDevice) {
        OutPipe_Winsock* outPipe = static_cast<OutPipe_Winsock*>(outPipe_);
        // FIXME: implement
    }
    return result;
}

OutPipe::Funcs OutPipe_Winsock::Funcs_ = {
    OutPipe_Winsock_destroy,
    OutPipe_Winsock_write,
    OutPipe_Winsock_flush,
    OutPipe::seek_Empty,
};

PLY_NO_INLINE OutPipe_Winsock::OutPipe_Winsock(SOCKET socket) : OutPipe{&Funcs_}, socket{socket} {
}

} // namespace ply

#endif // PLY_TARGET_WIN32

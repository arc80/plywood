/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-runtime/Precomp.h>

#if PLY_TARGET_WIN32

namespace ply {

// ┏━━━━━━━━━━━━━━━━━━┓
// ┃  InPipe_Winsock  ┃
// ┗━━━━━━━━━━━━━━━━━━┛
InPipe_Winsock::~InPipe_Winsock() {
    if (this->socket != INVALID_SOCKET) {
        int rc = closesocket(this->socket);
    }
}

u32 InPipe_Winsock::read(MutStringView buf) {
    int rc = recv(this->socket, buf.bytes, buf.num_bytes, 0);
    if (rc <= 0) // Handles SOCKET_ERROR.
        return 0;
    return rc;
}

// ┏━━━━━━━━━━━━━━━━━━━┓
// ┃  OutPipe_Winsock  ┃
// ┗━━━━━━━━━━━━━━━━━━━┛
OutPipe_Winsock::~OutPipe_Winsock() {
    if (this->socket != INVALID_SOCKET) {
        closesocket(this->socket);
    }
}

bool OutPipe_Winsock::write(StringView buf) {
    while (buf.num_bytes > 0) {
        int rc = send(this->socket, buf.bytes, buf.num_bytes, 0);
        if (u32(rc) > buf.num_bytes) // Handles SOCKET_ERROR.
            return false;
        buf.offset_head(rc);
    }
    return true;
}

} // namespace ply

#endif // PLY_TARGET_WIN32

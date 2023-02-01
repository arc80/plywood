/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-runtime/Precomp.h>

#if PLY_TARGET_WIN32
#include <ws2tcpip.h> // FIXME: Rewrite without using inet_ntop
#elif PLY_TARGET_POSIX
#include <arpa/inet.h> // FIXME: Rewrite without using CRT (inet_ntop)
#endif

namespace ply {

String IPAddress::toString() const {
    char buf[INET6_ADDRSTRLEN] = {0};
    if (this->version() == IPAddress::V4) {
        // FIXME: Rewrite without using CRT
        const char* r = inet_ntop(AF_INET, &this->netOrdered[3], buf, INET6_ADDRSTRLEN);
        PLY_ASSERT(r == buf);
        PLY_UNUSED(r);
    } else {
        const char* r = inet_ntop(AF_INET6, this, buf, INET6_ADDRSTRLEN);
        PLY_ASSERT(r == buf);
        PLY_UNUSED(r);
    }
    return buf;
}

} // namespace ply

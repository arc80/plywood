/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/network/IPAddress.h>

#if !defined(PLY_IMPL_SOCKET_PATH)
#if PLY_TARGET_WIN32
#define PLY_IMPL_SOCKET_PATH "impl/Socket_Winsock.h"
#define PLY_IMPL_SOCKET_TYPE Socket_Winsock
#elif PLY_TARGET_POSIX
#define PLY_IMPL_SOCKET_PATH "impl/Socket_POSIX.h"
#define PLY_IMPL_SOCKET_TYPE Socket_POSIX
#else
#define PLY_IMPL_SOCKET_PATH "*** Unable to select a default Socket implementation ***"
#endif
#endif

#include PLY_IMPL_SOCKET_PATH

namespace ply {

using Socket = PLY_IMPL_SOCKET_TYPE;
using TCPConnection = Socket::TCPConnection;
using TCPListener = Socket::TCPListener;

PLY_INLINE IPAddress IPAddress::resolveHostName(const StringView hostName, IPAddress::Version ipVersion) {
    return Socket::resolveHostName(hostName, ipVersion);
}

} // namespace ply

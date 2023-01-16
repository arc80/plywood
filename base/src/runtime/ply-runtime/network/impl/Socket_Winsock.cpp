
/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-runtime/Precomp.h>

#if PLY_TARGET_WIN32

#include <ply-runtime/network/impl/Socket_Winsock.h>
#include <ply-runtime/thread/Thread.h>
#include <ply-runtime/io/OutStream.h>

#define PLY_IPWINSOCK_ALLOW_UNKNOWN_ERRORS 0

namespace ply {

#define PLY_WITH_IPV6 1
#if PLY_WITH_IPV6
PLY_STATIC_ASSERT(sizeof(struct sockaddr_in) <= sizeof(struct sockaddr_in6));
#define PLY_IF_IPV6(v6expr, v4expr) v6expr
#else
#define PLY_IF_IPV4(v6expr, v4expr) v4expr
#endif

bool Socket_Winsock::IsInit = false;
bool Socket_Winsock::HasIPv6 = false;
ThreadLocal<IPResult> Socket_Winsock::lastResult_;

PLY_NO_INLINE void Socket_Winsock::initialize(IPAddress::Version ipVersion) {
    PLY_ASSERT(!IsInit);
    // Initialize Winsock
    WSADATA wsaData;
    int rc = WSAStartup(MAKEWORD(2, 2), &wsaData);
    PLY_ASSERT(rc == 0);
    PLY_UNUSED(rc);
    PLY_ASSERT(LOBYTE(wsaData.wVersion) == 2 && HIBYTE(wsaData.wVersion) == 2);
    IsInit = true;
}

PLY_NO_INLINE void Socket_Winsock::shutdown() {
    PLY_ASSERT(IsInit);
    int rc = WSACleanup();
    PLY_ASSERT(rc == 0);
    PLY_UNUSED(rc);
    IsInit = false;
}

PLY_NO_INLINE TCPConnection_Winsock::~TCPConnection_Winsock() {
    // Prevent double-deletion of file descriptor
    this->outPipe.socket = INVALID_SOCKET;
}

PLY_NO_INLINE Owned<TCPConnection_Winsock> TCPListener_Winsock::accept() {
    if (this->listenSocket == INVALID_SOCKET) {
        Socket_Winsock::lastResult_.store(IPResult::NoSocket);
        return nullptr;
    }

    struct PLY_IF_IPV6(sockaddr_in6, sockaddr_in) remoteAddr;
    socklen_t remoteAddrLen = sizeof(sockaddr_in);
    if (PLY_IF_IPV6(Socket_Winsock::HasIPv6, false)) {
        remoteAddrLen = sizeof(sockaddr_in6);
    }
    socklen_t passedAddrLen = remoteAddrLen;
    SOCKET hostSocket =
        ::accept(this->listenSocket, (struct sockaddr*) &remoteAddr, &remoteAddrLen);

    if (hostSocket == INVALID_SOCKET) {
        // FIXME: Check WSAGetLastError
        PLY_ASSERT(PLY_IPWINSOCK_ALLOW_UNKNOWN_ERRORS);
        Socket_Winsock::lastResult_.store(IPResult::Unknown);
        return nullptr;
    }

    PLY_ASSERT(passedAddrLen >= remoteAddrLen);
    TCPConnection_Winsock* tcpConn = new TCPConnection_Winsock;
#if PLY_WITH_IPV6
    if (Socket_Winsock::HasIPv6 && remoteAddrLen == sizeof(sockaddr_in6)) {
        PLY_ASSERT(remoteAddr.sin6_family == AF_INET6);
        memcpy(&tcpConn->remoteAddr_, &remoteAddr.sin6_addr, 16);
    } else
#endif
    {
        struct sockaddr_in* remoteAddrV4 = (struct sockaddr_in*) &remoteAddr;
        PLY_ASSERT(remoteAddrV4->sin_family == AF_INET);
        tcpConn->remoteAddr_ = IPAddress::fromIPv4(remoteAddrV4->sin_addr.s_addr);
    }
    tcpConn->remotePort_ = PLY_CONVERT_BIG_ENDIAN(remoteAddr.sin6_port);
    tcpConn->inPipe.socket = hostSocket;
    tcpConn->outPipe.socket = hostSocket;
    Socket_Winsock::lastResult_.store(IPResult::OK);
    return tcpConn;
}

PLY_NO_INLINE SOCKET createSocket(int type) {
    int family = AF_INET;
    if (PLY_IF_IPV6(Socket_Winsock::HasIPv6, false)) {
        family = AF_INET6;
    }
    SOCKET s = socket(family, type, 0);
    if (s == INVALID_SOCKET) {
        int err = WSAGetLastError();
        switch (err) {
            case 0: // Dummy case to prevent compiler warnings
            default: {
                PLY_ASSERT(PLY_IPWINSOCK_ALLOW_UNKNOWN_ERRORS); // FIXME: Recognize this code
                Socket_Winsock::lastResult_.store(IPResult::Unknown);
                break;
            }
        }
    }
    return s;
}

PLY_NO_INLINE TCPListener_Winsock Socket_Winsock::bindTCP(u16 port) {
    SOCKET listenSocket = createSocket(SOCK_STREAM);
    if (listenSocket == INVALID_SOCKET) { // lastResult_ is already set
        return {};
    }

    BOOL reuseAddr = TRUE;
    int rc = setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, (const char*) &reuseAddr,
                        sizeof(reuseAddr));
    PLY_ASSERT(rc == 0 || PLY_IPWINSOCK_ALLOW_UNKNOWN_ERRORS);

    struct PLY_IF_IPV6(sockaddr_in6, sockaddr_in) serverAddr;
    socklen_t serverAddrLen = sizeof(sockaddr_in);
#if PLY_WITH_IPV6
    if (Socket_Winsock::HasIPv6) {
        serverAddrLen = sizeof(sockaddr_in6);
        memset(&serverAddr, 0, serverAddrLen);
#if PLY_KERNEL_FREEBSD
        serverAddr.sin6_len = serverAddrLen;
#endif
        serverAddr.sin6_family = AF_INET6;
        serverAddr.sin6_addr = IN6ADDR_ANY_INIT;
        serverAddr.sin6_port = PLY_CONVERT_BIG_ENDIAN(port);
    } else
#endif
    {
        struct sockaddr_in* serverAddrV4 = (struct sockaddr_in*) &serverAddr;
        memset(serverAddrV4, 0, serverAddrLen);
#if PLY_KERNEL_FREEBSD
        serverAddr.sin_len = serverAddrLen;
#endif
        serverAddrV4->sin_family = AF_INET;
        serverAddrV4->sin_addr.s_addr = INADDR_ANY;
        serverAddrV4->sin_port = PLY_CONVERT_BIG_ENDIAN(port);
    }

    rc = bind(listenSocket, (struct sockaddr*) &serverAddr, serverAddrLen);
    if (rc == 0) {
        rc = listen(listenSocket, 1);
        if (rc == 0) {
            Socket_Winsock::lastResult_.store(IPResult::OK);
            return TCPListener_Winsock{listenSocket};
        } else {
            int err = WSAGetLastError();
            switch (err) {
                case 0: // Dummy case to prevent compiler warnings
                default: {
                    // FIXME: Recognize this error code
                    PLY_ASSERT(PLY_IPWINSOCK_ALLOW_UNKNOWN_ERRORS);
                    Socket_Winsock::lastResult_.store(IPResult::Unknown);
                    break;
                }
            }
        }
    } else {
        int err = WSAGetLastError();
        switch (err) {
            case 0: // Dummy case to prevent compiler warnings
            default: {
                // FIXME: Recognize this error code
                PLY_ASSERT(PLY_IPWINSOCK_ALLOW_UNKNOWN_ERRORS);
                Socket_Winsock::lastResult_.store(IPResult::Unknown);
                break;
            }
        }
    }

    // Failed
    rc = ::closesocket(listenSocket);
    PLY_ASSERT(rc == 0 || PLY_IPWINSOCK_ALLOW_UNKNOWN_ERRORS);
    PLY_UNUSED(rc);
    return {};
}

PLY_NO_INLINE Owned<TCPConnection_Winsock> Socket_Winsock::connectTCP(const IPAddress& address,
                                                                      u16 port) {
    SOCKET connectSocket = createSocket(SOCK_STREAM);
    if (connectSocket == INVALID_SOCKET) { // lastResult_ is already set
        return {};
    }

    struct PLY_IF_IPV6(sockaddr_in6, sockaddr_in) remoteAddr;
    socklen_t remoteAddrLen = sizeof(sockaddr_in);
#if PLY_WITH_IPV6
    if (Socket_Winsock::HasIPv6) {
        remoteAddrLen = sizeof(sockaddr_in6);
        memset(&remoteAddr, 0, remoteAddrLen);
#if PLY_KERNEL_FREEBSD
        remoteAddr.sin6_len = remoteAddrLen;
#endif
        remoteAddr.sin6_family = AF_INET6;
        memcpy(&remoteAddr.sin6_addr, &address, 16);
        remoteAddr.sin6_port = PLY_CONVERT_BIG_ENDIAN(port);
    } else
#endif
    {
        PLY_ASSERT(address.version() == IPAddress::V4);
        struct sockaddr_in* remoteAddrV4 = (struct sockaddr_in*) &remoteAddr;
        memset(remoteAddrV4, 0, remoteAddrLen);
#if PLY_KERNEL_FREEBSD
        serverAddr.sin_len = serverAddrLen;
#endif
        remoteAddrV4->sin_family = AF_INET;
        remoteAddrV4->sin_addr.s_addr = address.netOrdered[3];
        remoteAddrV4->sin_port = PLY_CONVERT_BIG_ENDIAN(port);
    }

    int rc = ::connect(connectSocket, (sockaddr*) &remoteAddr, remoteAddrLen);
    if (rc == 0) {
        TCPConnection_Winsock* tcpConn = new TCPConnection_Winsock;
        tcpConn->remoteAddr_ = address;
        tcpConn->remotePort_ = port;
        tcpConn->inPipe.socket = connectSocket;
        tcpConn->outPipe.socket = connectSocket;
        Socket_Winsock::lastResult_.store(IPResult::OK);
        return tcpConn;
    }

    int err = WSAGetLastError();
    switch (err) {
        case WSAECONNREFUSED: {
            Socket_Winsock::lastResult_.store(IPResult::Refused);
            break;
        }
        default: {
            PLY_ASSERT(PLY_IPWINSOCK_ALLOW_UNKNOWN_ERRORS); // FIXME: Recognize this error ode
            Socket_Winsock::lastResult_.store(IPResult::Unknown);
            break;
        }
    }
    rc = ::closesocket(connectSocket);
    PLY_ASSERT(rc == 0 || PLY_IPWINSOCK_ALLOW_UNKNOWN_ERRORS);
    PLY_UNUSED(rc);
    return nullptr;
}

PLY_NO_INLINE IPAddress Socket_Winsock::resolveHostName(StringView hostName,
                                                        IPAddress::Version ipVersion) {
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
#if PLY_WITH_IPV6
    if (ipVersion == IPAddress::V6) {
        hints.ai_family = AF_INET6;
        hints.ai_flags = AI_V4MAPPED | AI_ADDRCONFIG; // Fallback to V4 if no V6
    }
#endif
    struct addrinfo* res = nullptr;
    int rc = getaddrinfo(hostName.withNullTerminator().bytes, nullptr, &hints, &res);
    PLY_ASSERT(rc == 0);
    PLY_UNUSED(rc);
    struct addrinfo* best = nullptr;
    for (struct addrinfo* cur = res; cur; cur = cur->ai_next) {
#if PLY_WITH_IPV6
        if (cur->ai_family == AF_INET6 && ipVersion == IPAddress::V6) {
            if (!best || best->ai_family != AF_INET6) {
                best = cur;
            }
        }
#endif
        if (cur->ai_family == AF_INET) {
            if (!best) {
                best = cur;
            }
        }
    }

    IPAddress ipAddr;
    if (best) {
#if PLY_WITH_IPV6
        if (best->ai_family == AF_INET6) {
            PLY_ASSERT(best->ai_addrlen >= sizeof(sockaddr_in6));
            struct sockaddr_in6* resolvedAddr = (struct sockaddr_in6*) best->ai_addr;
            memcpy(&ipAddr, &resolvedAddr->sin6_addr, 16);
        } else
#endif
        {
            PLY_ASSERT(best->ai_addrlen >= sizeof(sockaddr_in));
            struct sockaddr_in* resolvedAddr = (struct sockaddr_in*) best->ai_addr;
            ipAddr = IPAddress::fromIPv4(resolvedAddr->sin_addr.s_addr);
        }
    }
    freeaddrinfo(res);
    return ipAddr;
}

} // namespace ply

#endif // PLY_TARGET_WIN32

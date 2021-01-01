/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-runtime/Precomp.h>

#if PLY_TARGET_POSIX

#include <ply-runtime/network/impl/Socket_POSIX.h>
#include <ply-runtime/thread/Thread.h>
#include <ply-runtime/io/text/StringWriter.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <ply-runtime/io/StdIO.h>

#define PLY_IPPOSIX_ALLOW_UNKNOWN_ERRORS 1

namespace ply {

#define PLY_WITH_IPV6 1
#if PLY_WITH_IPV6
PLY_STATIC_ASSERT(sizeof(struct sockaddr_in) <= sizeof(struct sockaddr_in6));
#define PLY_IF_IPV6(v6expr, v4expr) v6expr
#else
#define PLY_IF_IPV4(v6expr, v4expr) v4expr
#endif

bool Socket_POSIX::IsInit = false;
bool Socket_POSIX::HasIPv6 = false;
ThreadLocal<IPResult> Socket_POSIX::lastResult_;

PLY_NO_INLINE void Socket_POSIX::initialize(IPAddress::Version ipVersion) {
    // FIXME: Move this to some kind of generic Plywood initialization function, since this disables
    // SIGPIPE for all file descriptors, not just sockets, and we probably always want that. In
    // particular, we want that when communicating with a subprocess:
    signal(SIGPIPE, SIG_IGN);

    IsInit = true;

#if PLY_WITH_IPV6
    if (ipVersion == IPAddress::V6) {
        // FIXME: Is there a better way to test for IPv6 support?
        int testSocket = socket(AF_INET6, SOCK_STREAM, 0);
        if (testSocket >= 0) {
            Socket_POSIX::HasIPv6 = true;
            int rc = ::close(testSocket);
            PLY_ASSERT(rc == 0 || PLY_IPPOSIX_ALLOW_UNKNOWN_ERRORS);
            PLY_UNUSED(rc);
        }
    }
#endif
}

PLY_NO_INLINE void Socket_POSIX::shutdown() {
    PLY_ASSERT(IsInit);
    IsInit = false;
}

PLY_NO_INLINE TCPConnection_POSIX::~TCPConnection_POSIX() {
    // Prevent double-deletion of file descriptor
    this->outPipe.fd = -1;
}

PLY_NO_INLINE Owned<TCPConnection_POSIX> TCPListener_POSIX::accept() {
    if (this->listenSocket < 0) {
        Socket_POSIX::lastResult_.store(IPResult::NoSocket);
        return nullptr;
    }

    struct PLY_IF_IPV6(sockaddr_in6, sockaddr_in) remoteAddr;
    socklen_t remoteAddrLen = sizeof(sockaddr_in);
    if (PLY_IF_IPV6(Socket_POSIX::HasIPv6, false)) {
        remoteAddrLen = sizeof(sockaddr_in6);
    }
    socklen_t passedAddrLen = remoteAddrLen;
    int hostSocket = ::accept(this->listenSocket, (struct sockaddr*) &remoteAddr, &remoteAddrLen);

    if (hostSocket <= 0) {
        // FIXME: Check errno
        PLY_ASSERT(PLY_IPPOSIX_ALLOW_UNKNOWN_ERRORS);
        Socket_POSIX::lastResult_.store(IPResult::Unknown);
        return nullptr;
    }

    PLY_ASSERT(passedAddrLen >= remoteAddrLen);
    PLY_UNUSED(passedAddrLen);
    TCPConnection_POSIX* tcpConn = new TCPConnection_POSIX;
#if PLY_WITH_IPV6
    if (Socket_POSIX::HasIPv6 && remoteAddrLen == sizeof(sockaddr_in6)) {
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
    tcpConn->inPipe.fd = hostSocket;
    tcpConn->outPipe.fd = hostSocket;
    Socket_POSIX::lastResult_.store(IPResult::OK);
    return tcpConn;
}

PLY_NO_INLINE int createSocket(int type) {
    int family = AF_INET;
    if (PLY_IF_IPV6(Socket_POSIX::HasIPv6, false)) {
        family = AF_INET6;
    }
    int s = socket(family, type, 0);
    if (s < 0) {
        switch (errno) {
            case ENOBUFS:
            case ENOMEM:
            case ENFILE:
            case EMFILE: {
                Socket_POSIX::lastResult_.store(IPResult::NoSocket);
                break;
            }
            case EAFNOSUPPORT:
            case EINVAL:
            case EPROTONOSUPPORT:
                // Maybe fall back to IPv4 if this happens for IPv6?
            default: {
                PLY_ASSERT(PLY_IPPOSIX_ALLOW_UNKNOWN_ERRORS); // FIXME: Recognize this code
                Socket_POSIX::lastResult_.store(IPResult::Unknown);
                break;
            }
        }
    }
    return s;
}

PLY_NO_INLINE TCPListener_POSIX Socket_POSIX::bindTCP(u16 port) {
    int listenSocket = createSocket(SOCK_STREAM);
    if (listenSocket < 0) { // lastResult_ is already set
        return {};
    }

    int reuseAddr = 1;
    int rc = setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, &reuseAddr, sizeof(reuseAddr));
    PLY_ASSERT(rc == 0 || PLY_IPPOSIX_ALLOW_UNKNOWN_ERRORS);

    struct PLY_IF_IPV6(sockaddr_in6, sockaddr_in) serverAddr;
    socklen_t serverAddrLen = sizeof(sockaddr_in);
#if PLY_WITH_IPV6
    if (Socket_POSIX::HasIPv6) {
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
            Socket_POSIX::lastResult_.store(IPResult::OK);
            return TCPListener_POSIX{listenSocket};
        } else {
            switch (errno) {
                case EADDRINUSE: {
                    Socket_POSIX::lastResult_.store(IPResult::InUse);
                    break;
                }
                default: {
                    // FIXME: Recognize this errno
                    PLY_ASSERT(PLY_IPPOSIX_ALLOW_UNKNOWN_ERRORS);
                    Socket_POSIX::lastResult_.store(IPResult::Unknown);
                    break;
                }
            }
        }
    } else {
        switch (errno) {
            case EADDRINUSE: {
                Socket_POSIX::lastResult_.store(IPResult::InUse);
                break;
            }
            default: {
                // FIXME: Recognize this errno
                PLY_ASSERT(PLY_IPPOSIX_ALLOW_UNKNOWN_ERRORS);
                Socket_POSIX::lastResult_.store(IPResult::Unknown);
                break;
            }
        }
    }

    // Failed
    rc = ::close(listenSocket);
    PLY_ASSERT(rc == 0 || PLY_IPPOSIX_ALLOW_UNKNOWN_ERRORS);
    PLY_UNUSED(rc);
    return {};
}

PLY_NO_INLINE Owned<TCPConnection_POSIX> Socket_POSIX::connectTCP(const IPAddress& address,
                                                                  u16 port) {
    int connectSocket = createSocket(SOCK_STREAM);
    if (connectSocket < 0) { // lastResult_ is already set
        return {};
    }

    struct PLY_IF_IPV6(sockaddr_in6, sockaddr_in) remoteAddr;
    socklen_t remoteAddrLen = sizeof(sockaddr_in);
#if PLY_WITH_IPV6
    if (Socket_POSIX::HasIPv6) {
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
        TCPConnection_POSIX* tcpConn = new TCPConnection_POSIX;
        tcpConn->remoteAddr_ = address;
        tcpConn->remotePort_ = port;
        tcpConn->inPipe.fd = connectSocket;
        tcpConn->outPipe.fd = connectSocket;
        Socket_POSIX::lastResult_.store(IPResult::OK);
        return tcpConn;
    }

    switch (errno) {
        case ECONNREFUSED: {
            Socket_POSIX::lastResult_.store(IPResult::Refused);
            break;
        }
        case ENETUNREACH: {
            Socket_POSIX::lastResult_.store(IPResult::Unreachable);
            break;
        }
        default: {
            PLY_ASSERT(PLY_IPPOSIX_ALLOW_UNKNOWN_ERRORS); // FIXME: Recognize this code
            Socket_POSIX::lastResult_.store(IPResult::Unknown);
            break;
        }
    }
    rc = ::close(connectSocket);
    PLY_ASSERT(rc == 0 || PLY_IPPOSIX_ALLOW_UNKNOWN_ERRORS);
    PLY_UNUSED(rc);
    return nullptr;
}

PLY_NO_INLINE IPAddress Socket_POSIX::resolveHostName(const StringView hostName,
                                                      IPAddress::Version ipVersion) {
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
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

#endif // PLY_TARGET_POSIX

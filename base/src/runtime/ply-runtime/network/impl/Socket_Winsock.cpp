/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-runtime/Precomp.h>

#if PLY_TARGET_WIN32

#define PLY_IPWINSOCK_ALLOW_UNKNOWN_ERRORS 0

namespace ply {

#define PLY_WITH_IPV6 1
#if PLY_WITH_IPV6
PLY_STATIC_ASSERT(sizeof(struct sockaddr_in) <= sizeof(struct sockaddr_in6));
#define PLY_IF_IPV6(v6expr, v4expr) v6expr
#else
#define PLY_IF_IPV4(v6expr, v4expr) v4expr
#endif

bool Socket::IsInit = false;
bool Socket::HasIPv6 = false;
ThreadLocal<IPResult> Socket::last_result_;

PLY_NO_INLINE void Socket::initialize(IPAddress::Version ip_version) {
    PLY_ASSERT(!IsInit);
    // Initialize Winsock
    WSADATA wsa_data;
    int rc = WSAStartup(MAKEWORD(2, 2), &wsa_data);
    PLY_ASSERT(rc == 0);
    PLY_UNUSED(rc);
    PLY_ASSERT(LOBYTE(wsa_data.wVersion) == 2 && HIBYTE(wsa_data.wVersion) == 2);
    IsInit = true;
}

PLY_NO_INLINE void Socket::shutdown() {
    PLY_ASSERT(IsInit);
    int rc = WSACleanup();
    PLY_ASSERT(rc == 0);
    PLY_UNUSED(rc);
    IsInit = false;
}

PLY_NO_INLINE TCPConnection::~TCPConnection() {
    // Prevent double-deletion of file descriptor
    this->out_pipe.socket = INVALID_SOCKET;
}

PLY_NO_INLINE Owned<TCPConnection> TCPListener::accept() {
    if (this->listen_socket == INVALID_SOCKET) {
        Socket::last_result_.store(IPResult::NoSocket);
        return nullptr;
    }

    struct PLY_IF_IPV6(sockaddr_in6, sockaddr_in) remote_addr;
    socklen_t remote_addr_len = sizeof(sockaddr_in);
    if (PLY_IF_IPV6(Socket::HasIPv6, false)) {
        remote_addr_len = sizeof(sockaddr_in6);
    }
    socklen_t passed_addr_len = remote_addr_len;
    SOCKET host_socket = ::accept(this->listen_socket, (struct sockaddr*) &remote_addr,
                                  &remote_addr_len);

    if (host_socket == INVALID_SOCKET) {
        // FIXME: Check WSAGetLastError
        PLY_ASSERT(PLY_IPWINSOCK_ALLOW_UNKNOWN_ERRORS);
        Socket::last_result_.store(IPResult::Unknown);
        return nullptr;
    }

    PLY_ASSERT(passed_addr_len >= remote_addr_len);
    TCPConnection* tcp_conn = new TCPConnection;
#if PLY_WITH_IPV6
    if (Socket::HasIPv6 && remote_addr_len == sizeof(sockaddr_in6)) {
        PLY_ASSERT(remote_addr.sin6_family == AF_INET6);
        memcpy(&tcp_conn->remote_addr_, &remote_addr.sin6_addr, 16);
    } else
#endif
    {
        struct sockaddr_in* remote_addr_v4 = (struct sockaddr_in*) &remote_addr;
        PLY_ASSERT(remote_addr_v4->sin_family == AF_INET);
        tcp_conn->remote_addr_ = IPAddress::from_ipv4(remote_addr_v4->sin_addr.s_addr);
    }
    tcp_conn->remote_port_ = PLY_CONVERT_BIG_ENDIAN(remote_addr.sin6_port);
    tcp_conn->in_pipe.socket = host_socket;
    tcp_conn->out_pipe.socket = host_socket;
    Socket::last_result_.store(IPResult::OK);
    return tcp_conn;
}

PLY_NO_INLINE SOCKET create_socket(int type) {
    int family = AF_INET;
    if (PLY_IF_IPV6(Socket::HasIPv6, false)) {
        family = AF_INET6;
    }
    SOCKET s = socket(family, type, 0);
    if (s == INVALID_SOCKET) {
        int err = WSAGetLastError();
        switch (err) {
            case 0: // Dummy case to prevent compiler warnings
            default: {
                PLY_ASSERT(
                    PLY_IPWINSOCK_ALLOW_UNKNOWN_ERRORS); // FIXME: Recognize this code
                Socket::last_result_.store(IPResult::Unknown);
                break;
            }
        }
    }
    return s;
}

PLY_NO_INLINE TCPListener Socket::bind_tcp(u16 port) {
    SOCKET listen_socket = create_socket(SOCK_STREAM);
    if (listen_socket == INVALID_SOCKET) { // last_result_ is already set
        return {};
    }

    BOOL reuse_addr = TRUE;
    int rc = setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR,
                        (const char*) &reuse_addr, sizeof(reuse_addr));
    PLY_ASSERT(rc == 0 || PLY_IPWINSOCK_ALLOW_UNKNOWN_ERRORS);

    struct PLY_IF_IPV6(sockaddr_in6, sockaddr_in) server_addr;
    socklen_t server_addr_len = sizeof(sockaddr_in);
#if PLY_WITH_IPV6
    if (Socket::HasIPv6) {
        server_addr_len = sizeof(sockaddr_in6);
        memset(&server_addr, 0, server_addr_len);
#if PLY_KERNEL_FREEBSD
        server_addr.sin6_len = server_addr_len;
#endif
        server_addr.sin6_family = AF_INET6;
        server_addr.sin6_addr = IN6ADDR_ANY_INIT;
        server_addr.sin6_port = PLY_CONVERT_BIG_ENDIAN(port);
    } else
#endif
    {
        struct sockaddr_in* server_addr_v4 = (struct sockaddr_in*) &server_addr;
        memset(server_addr_v4, 0, server_addr_len);
#if PLY_KERNEL_FREEBSD
        server_addr.sin_len = server_addr_len;
#endif
        server_addr_v4->sin_family = AF_INET;
        server_addr_v4->sin_addr.s_addr = INADDR_ANY;
        server_addr_v4->sin_port = PLY_CONVERT_BIG_ENDIAN(port);
    }

    rc = bind(listen_socket, (struct sockaddr*) &server_addr, server_addr_len);
    if (rc == 0) {
        rc = listen(listen_socket, 1);
        if (rc == 0) {
            Socket::last_result_.store(IPResult::OK);
            return TCPListener{listen_socket};
        } else {
            int err = WSAGetLastError();
            switch (err) {
                case 0: // Dummy case to prevent compiler warnings
                default: {
                    // FIXME: Recognize this error code
                    PLY_ASSERT(PLY_IPWINSOCK_ALLOW_UNKNOWN_ERRORS);
                    Socket::last_result_.store(IPResult::Unknown);
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
                Socket::last_result_.store(IPResult::Unknown);
                break;
            }
        }
    }

    // Failed
    rc = ::closesocket(listen_socket);
    PLY_ASSERT(rc == 0 || PLY_IPWINSOCK_ALLOW_UNKNOWN_ERRORS);
    PLY_UNUSED(rc);
    return {};
}

PLY_NO_INLINE Owned<TCPConnection> Socket::connect_tcp(const IPAddress& address,
                                                       u16 port) {
    SOCKET connect_socket = create_socket(SOCK_STREAM);
    if (connect_socket == INVALID_SOCKET) { // last_result_ is already set
        return {};
    }

    struct PLY_IF_IPV6(sockaddr_in6, sockaddr_in) remote_addr;
    socklen_t remote_addr_len = sizeof(sockaddr_in);
#if PLY_WITH_IPV6
    if (Socket::HasIPv6) {
        remote_addr_len = sizeof(sockaddr_in6);
        memset(&remote_addr, 0, remote_addr_len);
#if PLY_KERNEL_FREEBSD
        remote_addr.sin6_len = remote_addr_len;
#endif
        remote_addr.sin6_family = AF_INET6;
        memcpy(&remote_addr.sin6_addr, &address, 16);
        remote_addr.sin6_port = PLY_CONVERT_BIG_ENDIAN(port);
    } else
#endif
    {
        PLY_ASSERT(address.version() == IPAddress::V4);
        struct sockaddr_in* remote_addr_v4 = (struct sockaddr_in*) &remote_addr;
        memset(remote_addr_v4, 0, remote_addr_len);
#if PLY_KERNEL_FREEBSD
        server_addr.sin_len = server_addr_len;
#endif
        remote_addr_v4->sin_family = AF_INET;
        remote_addr_v4->sin_addr.s_addr = address.net_ordered[3];
        remote_addr_v4->sin_port = PLY_CONVERT_BIG_ENDIAN(port);
    }

    int rc = ::connect(connect_socket, (sockaddr*) &remote_addr, remote_addr_len);
    if (rc == 0) {
        TCPConnection* tcp_conn = new TCPConnection;
        tcp_conn->remote_addr_ = address;
        tcp_conn->remote_port_ = port;
        tcp_conn->in_pipe.socket = connect_socket;
        tcp_conn->out_pipe.socket = connect_socket;
        Socket::last_result_.store(IPResult::OK);
        return tcp_conn;
    }

    int err = WSAGetLastError();
    switch (err) {
        case WSAECONNREFUSED: {
            Socket::last_result_.store(IPResult::Refused);
            break;
        }
        default: {
            PLY_ASSERT(
                PLY_IPWINSOCK_ALLOW_UNKNOWN_ERRORS); // FIXME: Recognize this error ode
            Socket::last_result_.store(IPResult::Unknown);
            break;
        }
    }
    rc = ::closesocket(connect_socket);
    PLY_ASSERT(rc == 0 || PLY_IPWINSOCK_ALLOW_UNKNOWN_ERRORS);
    PLY_UNUSED(rc);
    return nullptr;
}

PLY_NO_INLINE IPAddress Socket::resolve_host_name(StringView host_name,
                                                  IPAddress::Version ip_version) {
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
#if PLY_WITH_IPV6
    if (ip_version == IPAddress::V6) {
        hints.ai_family = AF_INET6;
        hints.ai_flags = AI_V4MAPPED | AI_ADDRCONFIG; // Fallback to V4 if no V6
    }
#endif
    struct addrinfo* res = nullptr;
    int rc = getaddrinfo(host_name.with_null_terminator().bytes, nullptr, &hints, &res);
    PLY_ASSERT(rc == 0);
    PLY_UNUSED(rc);
    struct addrinfo* best = nullptr;
    for (struct addrinfo* cur = res; cur; cur = cur->ai_next) {
#if PLY_WITH_IPV6
        if (cur->ai_family == AF_INET6 && ip_version == IPAddress::V6) {
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

    IPAddress ip_addr;
    if (best) {
#if PLY_WITH_IPV6
        if (best->ai_family == AF_INET6) {
            PLY_ASSERT(best->ai_addrlen >= sizeof(sockaddr_in6));
            struct sockaddr_in6* resolved_addr = (struct sockaddr_in6*) best->ai_addr;
            memcpy(&ip_addr, &resolved_addr->sin6_addr, 16);
        } else
#endif
        {
            PLY_ASSERT(best->ai_addrlen >= sizeof(sockaddr_in));
            struct sockaddr_in* resolved_addr = (struct sockaddr_in*) best->ai_addr;
            ip_addr = IPAddress::from_ipv4(resolved_addr->sin_addr.s_addr);
        }
    }
    freeaddrinfo(res);
    return ip_addr;
}

} // namespace ply

#endif // PLY_TARGET_WIN32

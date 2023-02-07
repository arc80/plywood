/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-runtime/Precomp.h>

#if PLY_TARGET_POSIX

#include <ply-runtime/network/impl/Socket_POSIX.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

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
ThreadLocal<IPResult> Socket_POSIX::last_result_;

void Socket_POSIX::initialize(IPAddress::Version ip_version) {
    // FIXME: Move this to some kind of generic Plywood initialization function, since
    // this disables SIGPIPE for all file descriptors, not just sockets, and we probably
    // always want that. In particular, we want that when communicating with a
    // subprocess:
    signal(SIGPIPE, SIG_IGN);

    IsInit = true;

#if PLY_WITH_IPV6
    if (ip_version == IPAddress::V6) {
        // FIXME: Is there a better way to test for IPv6 support?
        int test_socket = socket(AF_INET6, SOCK_STREAM, 0);
        if (test_socket >= 0) {
            Socket_POSIX::HasIPv6 = true;
            int rc = ::close(test_socket);
            PLY_ASSERT(rc == 0 || PLY_IPPOSIX_ALLOW_UNKNOWN_ERRORS);
            PLY_UNUSED(rc);
        }
    }
#endif
}

void Socket_POSIX::shutdown() {
    PLY_ASSERT(IsInit);
    IsInit = false;
}

TCPConnection_POSIX::~TCPConnection_POSIX() {
    // Prevent double-deletion of file descriptor
    this->out_pipe.fd = -1;
}

Owned<TCPConnection_POSIX> TCPListener_POSIX::accept() {
    if (this->listen_socket < 0) {
        Socket_POSIX::last_result_.store(IPResult::NoSocket);
        return nullptr;
    }

    struct PLY_IF_IPV6(sockaddr_in6, sockaddr_in) remote_addr;
    socklen_t remote_addr_len = sizeof(sockaddr_in);
    if (PLY_IF_IPV6(Socket_POSIX::HasIPv6, false)) {
        remote_addr_len = sizeof(sockaddr_in6);
    }
    socklen_t passed_addr_len = remote_addr_len;
    int host_socket = ::accept(this->listen_socket, (struct sockaddr*) &remote_addr,
                               &remote_addr_len);

    if (host_socket <= 0) {
        // FIXME: Check errno
        PLY_ASSERT(PLY_IPPOSIX_ALLOW_UNKNOWN_ERRORS);
        Socket_POSIX::last_result_.store(IPResult::Unknown);
        return nullptr;
    }

    PLY_ASSERT(passed_addr_len >= remote_addr_len);
    PLY_UNUSED(passed_addr_len);
    TCPConnection_POSIX* tcp_conn = new TCPConnection_POSIX;
#if PLY_WITH_IPV6
    if (Socket_POSIX::HasIPv6 && remote_addr_len == sizeof(sockaddr_in6)) {
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
    tcp_conn->in_pipe.fd = host_socket;
    tcp_conn->out_pipe.fd = host_socket;
    Socket_POSIX::last_result_.store(IPResult::OK);
    return tcp_conn;
}

int create_socket(int type) {
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
                Socket_POSIX::last_result_.store(IPResult::NoSocket);
                break;
            }
            case EAFNOSUPPORT:
            case EINVAL:
            case EPROTONOSUPPORT:
                // Maybe fall back to IPv4 if this happens for IPv6?
            default: {
                PLY_ASSERT(
                    PLY_IPPOSIX_ALLOW_UNKNOWN_ERRORS); // FIXME: Recognize this code
                Socket_POSIX::last_result_.store(IPResult::Unknown);
                break;
            }
        }
    }
    return s;
}

TCPListener_POSIX Socket_POSIX::bind_tcp(u16 port) {
    int listen_socket = create_socket(SOCK_STREAM);
    if (listen_socket < 0) { // last_result_ is already set
        return {};
    }

    int reuse_addr = 1;
    int rc = setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, &reuse_addr,
                        sizeof(reuse_addr));
    PLY_ASSERT(rc == 0 || PLY_IPPOSIX_ALLOW_UNKNOWN_ERRORS);

    struct PLY_IF_IPV6(sockaddr_in6, sockaddr_in) server_addr;
    socklen_t server_addr_len = sizeof(sockaddr_in);
#if PLY_WITH_IPV6
    if (Socket_POSIX::HasIPv6) {
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
            Socket_POSIX::last_result_.store(IPResult::OK);
            return TCPListener_POSIX{listen_socket};
        } else {
            switch (errno) {
                case EADDRINUSE: {
                    Socket_POSIX::last_result_.store(IPResult::InUse);
                    break;
                }
                default: {
                    // FIXME: Recognize this errno
                    PLY_ASSERT(PLY_IPPOSIX_ALLOW_UNKNOWN_ERRORS);
                    Socket_POSIX::last_result_.store(IPResult::Unknown);
                    break;
                }
            }
        }
    } else {
        switch (errno) {
            case EADDRINUSE: {
                Socket_POSIX::last_result_.store(IPResult::InUse);
                break;
            }
            default: {
                // FIXME: Recognize this errno
                PLY_ASSERT(PLY_IPPOSIX_ALLOW_UNKNOWN_ERRORS);
                Socket_POSIX::last_result_.store(IPResult::Unknown);
                break;
            }
        }
    }

    // Failed
    rc = ::close(listen_socket);
    PLY_ASSERT(rc == 0 || PLY_IPPOSIX_ALLOW_UNKNOWN_ERRORS);
    PLY_UNUSED(rc);
    return {};
}

Owned<TCPConnection_POSIX> Socket_POSIX::connect_tcp(const IPAddress& address,
                                                     u16 port) {
    int connect_socket = create_socket(SOCK_STREAM);
    if (connect_socket < 0) { // last_result_ is already set
        return {};
    }

    struct PLY_IF_IPV6(sockaddr_in6, sockaddr_in) remote_addr;
    socklen_t remote_addr_len = sizeof(sockaddr_in);
#if PLY_WITH_IPV6
    if (Socket_POSIX::HasIPv6) {
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
        TCPConnection_POSIX* tcp_conn = new TCPConnection_POSIX;
        tcp_conn->remote_addr_ = address;
        tcp_conn->remote_port_ = port;
        tcp_conn->in_pipe.fd = connect_socket;
        tcp_conn->out_pipe.fd = connect_socket;
        Socket_POSIX::last_result_.store(IPResult::OK);
        return tcp_conn;
    }

    switch (errno) {
        case ECONNREFUSED: {
            Socket_POSIX::last_result_.store(IPResult::Refused);
            break;
        }
        case ENETUNREACH: {
            Socket_POSIX::last_result_.store(IPResult::Unreachable);
            break;
        }
        default: {
            PLY_ASSERT(PLY_IPPOSIX_ALLOW_UNKNOWN_ERRORS); // FIXME: Recognize this code
            Socket_POSIX::last_result_.store(IPResult::Unknown);
            break;
        }
    }
    rc = ::close(connect_socket);
    PLY_ASSERT(rc == 0 || PLY_IPPOSIX_ALLOW_UNKNOWN_ERRORS);
    PLY_UNUSED(rc);
    return nullptr;
}

IPAddress Socket_POSIX::resolve_host_name(StringView host_name,
                                          IPAddress::Version ip_version) {
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
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

#endif // PLY_TARGET_POSIX

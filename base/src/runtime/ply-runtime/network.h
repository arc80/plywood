/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once

#if PLY_TARGET_WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#elif PLY_TARGET_POSIX
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#endif

namespace ply {

//  ▄▄▄▄ ▄▄▄▄▄   ▄▄▄▄      ▄▄     ▄▄
//   ██  ██  ██ ██  ██  ▄▄▄██  ▄▄▄██ ▄▄▄▄▄   ▄▄▄▄   ▄▄▄▄   ▄▄▄▄
//   ██  ██▀▀▀  ██▀▀██ ██  ██ ██  ██ ██  ▀▀ ██▄▄██ ▀█▄▄▄  ▀█▄▄▄
//  ▄██▄ ██     ██  ██ ▀█▄▄██ ▀█▄▄██ ██     ▀█▄▄▄   ▄▄▄█▀  ▄▄▄█▀
//

struct IPAddress {
    enum Version {
        V4,
        V6,
    };

    u32 net_ordered[4]; // big endian

    Version version() const {
        return (this->net_ordered[0] == 0 && this->net_ordered[1] == 0 &&
                this->net_ordered[2] == PLY_CONVERT_BIG_ENDIAN(0xffffu))
                   ? V4
                   : V6;
    }

    bool is_null() const {
        return this->net_ordered[0] == 0 && this->net_ordered[1] == 0 &&
               this->net_ordered[2] == 0 && this->net_ordered[3] == 0;
    }

    static constexpr IPAddress local_host(Version ip_version) {
        return (ip_version == Version::V4)
                   ? IPAddress{{0, 0, PLY_CONVERT_BIG_ENDIAN(0xffffu),
                                PLY_CONVERT_BIG_ENDIAN(0x7f000001u)}}
                   : IPAddress{{0, 0, 0, PLY_CONVERT_BIG_ENDIAN(1u)}};
    }

    static constexpr IPAddress from_ipv4(u32 net_ordered) {
        return {{0, 0, PLY_CONVERT_BIG_ENDIAN(0xffffu), net_ordered}};
    }

    String to_string() const;
    static IPAddress from_string();
    static IPAddress resolve_host_name(StringView host_name, Version ip_version);
};

enum class IPResult : u8 {
    Unknown = 0,
    OK,
    NoSocket,
    Unreachable,
    Refused,
    InUse,
};

//   ▄▄▄▄               ▄▄             ▄▄
//  ██  ▀▀  ▄▄▄▄   ▄▄▄▄ ██  ▄▄  ▄▄▄▄  ▄██▄▄
//   ▀▀▀█▄ ██  ██ ██    ██▄█▀  ██▄▄██  ██
//  ▀█▄▄█▀ ▀█▄▄█▀ ▀█▄▄▄ ██ ▀█▄ ▀█▄▄▄   ▀█▄▄
//

struct TCPListener;
struct TCPConnection;

#if PLY_TARGET_WIN32

// ┏━━━━━━━━━━━━━━━━━━┓
// ┃  InPipe_Winsock  ┃
// ┗━━━━━━━━━━━━━━━━━━┛
struct InPipe_Winsock : InPipe {
    static constexpr char* Type = "Winsock";
    SOCKET socket = INVALID_SOCKET;

    InPipe_Winsock(SOCKET s) : InPipe{Type}, socket{s} {
    }
    virtual ~InPipe_Winsock();
    virtual u32 read(MutStringView buf) override;
};

// ┏━━━━━━━━━━━━━━━━━━━┓
// ┃  OutPipe_Winsock  ┃
// ┗━━━━━━━━━━━━━━━━━━━┛
struct OutPipe_Winsock : OutPipe {
    static constexpr char* Type = "Winsock";
    SOCKET socket = INVALID_SOCKET;

    OutPipe_Winsock(SOCKET s) : OutPipe{Type}, socket{s} {
    }
    virtual ~OutPipe_Winsock();
    virtual bool write(StringView buf) override;
};

// ┏━━━━━━━━━━━┓
// ┃  Winsock  ┃
// ┗━━━━━━━━━━━┛
struct Socket {
    using Handle = SOCKET;

    static constexpr Handle InvalidHandle = INVALID_SOCKET;
    static bool IsInit;
    static bool HasIPv6;
    static ThreadLocal<IPResult> last_result_;

    static void initialize(IPAddress::Version ip_version);
    static void shutdown();

    // FIXME: Make interface more configurable
    static TCPListener bind_tcp(u16 port);
    static Owned<TCPConnection> connect_tcp(const IPAddress& address, u16 port);
    static IPAddress resolve_host_name(StringView host_name,
                                       IPAddress::Version ip_version);
    static IPResult last_result() {
        return Socket::last_result_.load();
    }
};

#elif PLY_TARGET_POSIX

// ┏━━━━━━━━━┓
// ┃  POSIX  ┃
// ┗━━━━━━━━━┛
struct Socket {
    using Handle = int;

    static constexpr Handle InvalidHandle = -1;
    static bool IsInit;
    static bool HasIPv6;
    static ThreadLocal<IPResult> last_result_;

    static void initialize(IPAddress::Version ip_version);
    static void shutdown();

    // FIXME: Make interface more configurable
    static TCPListener bind_tcp(u16 port);
    static Owned<TCPConnection> connect_tcp(const IPAddress& address, u16 port);
    static IPAddress resolve_host_name(StringView host_name,
                                       IPAddress::Version ip_version);
    static IPResult last_result() {
        return Socket::last_result_.load();
    }
};

#endif

inline IPAddress IPAddress::resolve_host_name(StringView host_name,
                                              IPAddress::Version ip_version) {
    return Socket::resolve_host_name(host_name, ip_version);
}

//  ▄▄▄▄▄▄  ▄▄▄▄  ▄▄▄▄▄   ▄▄▄▄                                     ▄▄   ▄▄
//    ██   ██  ▀▀ ██  ██ ██  ▀▀  ▄▄▄▄  ▄▄▄▄▄  ▄▄▄▄▄   ▄▄▄▄   ▄▄▄▄ ▄██▄▄ ▄▄  ▄▄▄▄  ▄▄▄▄▄
//    ██   ██     ██▀▀▀  ██     ██  ██ ██  ██ ██  ██ ██▄▄██ ██     ██   ██ ██  ██ ██  ██
//    ██   ▀█▄▄█▀ ██     ▀█▄▄█▀ ▀█▄▄█▀ ██  ██ ██  ██ ▀█▄▄▄  ▀█▄▄▄  ▀█▄▄ ██ ▀█▄▄█▀ ██  ██
//

#if PLY_TARGET_WIN32
// ┏━━━━━━━━━━━┓
// ┃  Winsock  ┃
// ┗━━━━━━━━━━━┛
struct TCPConnection {
    IPAddress remote_addr_;
    u16 remote_port_ = 0;
    InPipe_Winsock in_pipe;
    OutPipe_Winsock out_pipe;

    TCPConnection() : in_pipe{INVALID_SOCKET}, out_pipe{INVALID_SOCKET} {
    }
    ~TCPConnection();
    const IPAddress& remote_address() const {
        return this->remote_addr_;
    }
    u16 remote_port() const {
        return this->remote_port_;
    }
    SOCKET get_handle() const {
        return in_pipe.socket;
    }
    InStream create_in_stream() {
        return InStream{&this->in_pipe, false};
    }
    OutStream create_out_stream() {
        return OutStream{&this->out_pipe, false};
    }
};

#elif PLY_TARGET_POSIX
// ┏━━━━━━━━━┓
// ┃  POSIX  ┃
// ┗━━━━━━━━━┛
struct TCPConnection {
    IPAddress remote_addr_;
    u16 remote_port_ = 0;
    InPipe_FD in_pipe;
    OutPipe_FD out_pipe;

    TCPConnection() : in_pipe{-1}, out_pipe{-1} {
    }
    ~TCPConnection();
    const IPAddress& remote_address() const {
        return this->remote_addr_;
    }
    u16 remote_port() const {
        return this->remote_port_;
    }
    int get_handle() const {
        return in_pipe.fd;
    }
    InStream create_in_stream() {
        return InStream{borrow(&this->in_pipe)};
    }
    OutStream create_out_stream() {
        return OutStream{borrow(&this->out_pipe)};
    }
};

#endif // TCPConnection

//  ▄▄▄▄▄▄  ▄▄▄▄  ▄▄▄▄▄  ▄▄    ▄▄         ▄▄
//    ██   ██  ▀▀ ██  ██ ██    ▄▄  ▄▄▄▄  ▄██▄▄  ▄▄▄▄  ▄▄▄▄▄   ▄▄▄▄  ▄▄▄▄▄
//    ██   ██     ██▀▀▀  ██    ██ ▀█▄▄▄   ██   ██▄▄██ ██  ██ ██▄▄██ ██  ▀▀
//    ██   ▀█▄▄█▀ ██     ██▄▄▄ ██  ▄▄▄█▀  ▀█▄▄ ▀█▄▄▄  ██  ██ ▀█▄▄▄  ██
//

#if PLY_TARGET_WIN32
// ┏━━━━━━━━━━━┓
// ┃  Winsock  ┃
// ┗━━━━━━━━━━━┛
struct TCPListener {
public:
    SOCKET listen_socket = INVALID_SOCKET;

    TCPListener(SOCKET listen_socket = INVALID_SOCKET) : listen_socket{listen_socket} {
    }
    TCPListener(TCPListener&& other) {
        this->listen_socket = other.listen_socket;
        other.listen_socket = INVALID_SOCKET;
    }
    ~TCPListener() {
        if (this->listen_socket >= 0) {
            closesocket(this->listen_socket);
        }
    }
    void operator=(TCPListener&& other) {
        if (this->listen_socket >= 0) {
            closesocket(this->listen_socket);
        }
        this->listen_socket = other.listen_socket;
        other.listen_socket = INVALID_SOCKET;
    }
    bool is_valid() {
        return this->listen_socket >= 0;
    }
    void end_comm() {
        shutdown(this->listen_socket, SD_BOTH);
    }
    void close() {
        if (this->listen_socket >= 0) {
            closesocket(this->listen_socket);
            this->listen_socket = INVALID_SOCKET;
        }
    }

    Owned<TCPConnection> accept();
};

#elif PLY_TARGET_POSIX
// ┏━━━━━━━━━┓
// ┃  POSIX  ┃
// ┗━━━━━━━━━┛
struct TCPListener {
public:
    int listen_socket = -1;

    TCPListener(int listen_socket = -1) : listen_socket{listen_socket} {
    }
    TCPListener(TCPListener&& other) {
        this->listen_socket = other.listen_socket;
        other.listen_socket = -1;
    }
    ~TCPListener() {
        if (this->listen_socket >= 0) {
            ::close(this->listen_socket);
        }
    }
    void operator=(TCPListener&& other) {
        if (this->listen_socket >= 0) {
            ::close(this->listen_socket);
        }
        this->listen_socket = other.listen_socket;
        other.listen_socket = -1;
    }
    bool is_valid() {
        return this->listen_socket >= 0;
    }
    void end_comm() {
        shutdown(this->listen_socket, SHUT_RDWR);
    }
    void close() {
        if (this->listen_socket >= 0) {
            ::close(this->listen_socket);
            this->listen_socket = -1;
        }
    }

    Owned<TCPConnection_POSIX> accept();
};

#endif // TCPListener

} // namespace ply

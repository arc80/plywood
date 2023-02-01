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

  u32 netOrdered[4]; // big endian

  Version version() const {
    return (this->netOrdered[0] == 0 && this->netOrdered[1] == 0 &&
            this->netOrdered[2] == PLY_CONVERT_BIG_ENDIAN(0xffffu))
               ? V4
               : V6;
  }

  bool isNull() const {
    return this->netOrdered[0] == 0 && this->netOrdered[1] == 0 &&
           this->netOrdered[2] == 0 && this->netOrdered[3] == 0;
  }

  static constexpr IPAddress localHost(Version ipVersion) {
    return (ipVersion == Version::V4)
               ? IPAddress{{0, 0, PLY_CONVERT_BIG_ENDIAN(0xffffu),
                            PLY_CONVERT_BIG_ENDIAN(0x7f000001u)}}
               : IPAddress{{0, 0, 0, PLY_CONVERT_BIG_ENDIAN(1u)}};
  }

  static constexpr IPAddress fromIPv4(u32 netOrdered) {
    return {{0, 0, PLY_CONVERT_BIG_ENDIAN(0xffffu), netOrdered}};
  }

  String toString() const;
  static IPAddress fromString();
  static IPAddress resolveHostName(StringView hostName,
                                              Version ipVersion);
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
    static ThreadLocal<IPResult> lastResult_;

    static void initialize(IPAddress::Version ipVersion);
    static void shutdown();

    // FIXME: Make interface more configurable
    static TCPListener bindTCP(u16 port);
    static Owned<TCPConnection> connectTCP(const IPAddress& address,
                                                                 u16 port);
    static IPAddress resolveHostName(StringView hostName,
                                                   IPAddress::Version ipVersion);
    static IPResult lastResult() {
        return Socket::lastResult_.load();
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
    static ThreadLocal<IPResult> lastResult_;

    static PLY_DLL_ENTRY void initialize(IPAddress::Version ipVersion);
    static PLY_DLL_ENTRY void shutdown();

    // FIXME: Make interface more configurable
    static PLY_DLL_ENTRY TCPListener bindTCP(u16 port);
    static PLY_DLL_ENTRY Owned<TCPConnection> connectTCP(const IPAddress& address, u16 port);
    static PLY_DLL_ENTRY IPAddress resolveHostName(StringView hostName,
                                                   IPAddress::Version ipVersion);
    static PLY_INLINE IPResult lastResult() {
        return Socket::lastResult_.load();
    }
};

#endif

PLY_INLINE IPAddress IPAddress::resolveHostName(StringView hostName, IPAddress::Version ipVersion) {
    return Socket::resolveHostName(hostName, ipVersion);
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
    IPAddress remoteAddr_;
    u16 remotePort_ = 0;
    InPipe_Winsock inPipe;
    OutPipe_Winsock outPipe;

    TCPConnection() : inPipe{INVALID_SOCKET}, outPipe{INVALID_SOCKET} {
    }
    ~TCPConnection();
    const IPAddress& remoteAddress() const {
        return this->remoteAddr_;
    }
    u16 remotePort() const {
        return this->remotePort_;
    }
    SOCKET getHandle() const {
        return inPipe.socket;
    }
    InStream createInStream() {
        return InStream{&this->inPipe, false};
    }
    OutStream createOutStream() {
        return OutStream{&this->outPipe, false};
    }
};

#elif PLY_TARGET_POSIX
// ┏━━━━━━━━━┓
// ┃  POSIX  ┃
// ┗━━━━━━━━━┛
struct TCPConnection {
    IPAddress remoteAddr_;
    u16 remotePort_ = 0;
    InPipe_FD inPipe;
    OutPipe_FD outPipe;

    PLY_INLINE TCPConnection() : inPipe{-1}, outPipe{-1} {
    }
    PLY_DLL_ENTRY ~TCPConnection();
    PLY_INLINE const IPAddress& remoteAddress() const {
        return this->remoteAddr_;
    }
    PLY_INLINE u16 remotePort() const {
        return this->remotePort_;
    }
    PLY_INLINE int getHandle() const {
        return inPipe.fd;
    }
    PLY_INLINE InStream createInStream() {
        return InStream{borrow(&this->inPipe)};
    }
    PLY_INLINE OutStream createOutStream() {
        return OutStream{borrow(&this->outPipe)};
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
    SOCKET listenSocket = INVALID_SOCKET;

    TCPListener(SOCKET listenSocket = INVALID_SOCKET)
        : listenSocket{listenSocket} {
    }
    TCPListener(TCPListener&& other) {
        this->listenSocket = other.listenSocket;
        other.listenSocket = INVALID_SOCKET;
    }
    ~TCPListener() {
        if (this->listenSocket >= 0) {
            closesocket(this->listenSocket);
        }
    }
    void operator=(TCPListener&& other) {
        if (this->listenSocket >= 0) {
            closesocket(this->listenSocket);
        }
        this->listenSocket = other.listenSocket;
        other.listenSocket = INVALID_SOCKET;
    }
    bool isValid() {
        return this->listenSocket >= 0;
    }
    void endComm() {
        shutdown(this->listenSocket, SD_BOTH);
    }
    void close() {
        if (this->listenSocket >= 0) {
            closesocket(this->listenSocket);
            this->listenSocket = INVALID_SOCKET;
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
    int listenSocket = -1;

    PLY_INLINE TCPListener(int listenSocket = -1) : listenSocket{listenSocket} {
    }
    PLY_INLINE TCPListener(TCPListener&& other) {
        this->listenSocket = other.listenSocket;
        other.listenSocket = -1;
    }
    PLY_INLINE ~TCPListener() {
        if (this->listenSocket >= 0) {
            ::close(this->listenSocket);
        }
    }
    PLY_INLINE void operator=(TCPListener&& other) {
        if (this->listenSocket >= 0) {
            ::close(this->listenSocket);
        }
        this->listenSocket = other.listenSocket;
        other.listenSocket = -1;
    }
    PLY_INLINE bool isValid() {
        return this->listenSocket >= 0;
    }
    PLY_INLINE void endComm() {
        shutdown(this->listenSocket, SHUT_RDWR);
    }
    PLY_INLINE void close() {
        if (this->listenSocket >= 0) {
            ::close(this->listenSocket);
            this->listenSocket = -1;
        }
    }

    PLY_DLL_ENTRY Owned<TCPConnection_POSIX> accept();
};

#endif // TCPListener

} // namespace ply

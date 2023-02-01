/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime.h>
#include <ply-runtime/network/IPAddress.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

namespace ply {

struct TCPListener_POSIX;
struct TCPConnection_POSIX;

//------------------------------------------------------------------
// Socket_POSIX
//------------------------------------------------------------------
struct Socket_POSIX {
    using Handle = int;
    using InPipe = InPipe_FD;
    using OutPipe = OutPipe_FD;
    using TCPListener = TCPListener_POSIX;
    using TCPConnection = TCPConnection_POSIX;

    static constexpr Handle InvalidHandle = -1;
    static bool IsInit;
    static bool HasIPv6;
    static ThreadLocal<IPResult> lastResult_;

    static PLY_DLL_ENTRY void initialize(IPAddress::Version ipVersion);
    static PLY_DLL_ENTRY void shutdown();

    // FIXME: Make interface more configurable
    static PLY_DLL_ENTRY TCPListener_POSIX bindTCP(u16 port);
    static PLY_DLL_ENTRY Owned<TCPConnection_POSIX> connectTCP(const IPAddress& address, u16 port);
    static PLY_DLL_ENTRY IPAddress resolveHostName(StringView hostName,
                                                   IPAddress::Version ipVersion);
    static PLY_INLINE IPResult lastResult() {
        return Socket_POSIX::lastResult_.load();
    }
};

//------------------------------------------------------------------
// TCPConnection_POSIX
//------------------------------------------------------------------
struct TCPConnection_POSIX {
    IPAddress remoteAddr_;
    u16 remotePort_ = 0;
    InPipe_FD inPipe;
    OutPipe_FD outPipe;

    PLY_INLINE TCPConnection_POSIX() : inPipe{-1}, outPipe{-1} {
    }
    PLY_DLL_ENTRY ~TCPConnection_POSIX();
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

//------------------------------------------------------------------
// TCPListener_POSIX
//------------------------------------------------------------------
struct TCPListener_POSIX {
public:
    int listenSocket = -1;

    PLY_INLINE TCPListener_POSIX(int listenSocket = -1) : listenSocket{listenSocket} {
    }
    PLY_INLINE TCPListener_POSIX(TCPListener_POSIX&& other) {
        this->listenSocket = other.listenSocket;
        other.listenSocket = -1;
    }
    PLY_INLINE ~TCPListener_POSIX() {
        if (this->listenSocket >= 0) {
            ::close(this->listenSocket);
        }
    }
    PLY_INLINE void operator=(TCPListener_POSIX&& other) {
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

} // namespace ply

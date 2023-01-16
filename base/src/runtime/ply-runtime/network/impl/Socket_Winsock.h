/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/network/IPAddress.h>
#include <ply-runtime/io/impl/Pipe_Winsock.h>
#include <ply-runtime/container/Owned.h>
#include <ply-runtime/io/InStream.h>
#include <ply-runtime/io/OutStream.h>
#include <ply-runtime/thread/ThreadLocal.h>
#include <ws2tcpip.h>

namespace ply {

struct TCPListener_Winsock;
struct TCPConnection_Winsock;

//------------------------------------------------------------------
// Socket_Winsock
//------------------------------------------------------------------
struct Socket_Winsock {
    using Handle = SOCKET;
    using InPipe = InPipe_Winsock;
    using OutPipe = OutPipe_Winsock;
    using TCPListener = TCPListener_Winsock;
    using TCPConnection = TCPConnection_Winsock;

    static constexpr Handle InvalidHandle = INVALID_SOCKET;
    static bool IsInit;
    static bool HasIPv6;
    static ThreadLocal<IPResult> lastResult_;

    static PLY_DLL_ENTRY void initialize(IPAddress::Version ipVersion);
    static PLY_DLL_ENTRY void shutdown();

    // FIXME: Make interface more configurable
    static PLY_DLL_ENTRY TCPListener_Winsock bindTCP(u16 port);
    static PLY_DLL_ENTRY Owned<TCPConnection_Winsock> connectTCP(const IPAddress& address,
                                                                 u16 port);
    static PLY_DLL_ENTRY IPAddress resolveHostName(StringView hostName,
                                                   IPAddress::Version ipVersion);
    static PLY_INLINE IPResult lastResult() {
        return Socket_Winsock::lastResult_.load();
    }
};

//------------------------------------------------------------------
// TCPConnection_Winsock
//------------------------------------------------------------------
struct TCPConnection_Winsock {
    IPAddress remoteAddr_;
    u16 remotePort_ = 0;
    InPipe_Winsock inPipe;
    OutPipe_Winsock outPipe;

    PLY_INLINE TCPConnection_Winsock() : inPipe{INVALID_SOCKET}, outPipe{INVALID_SOCKET} {
    }
    PLY_DLL_ENTRY ~TCPConnection_Winsock();
    PLY_INLINE const IPAddress& remoteAddress() const {
        return this->remoteAddr_;
    }
    PLY_INLINE u16 remotePort() const {
        return this->remotePort_;
    }
    PLY_INLINE SOCKET getHandle() const {
        return inPipe.socket;
    }
    PLY_INLINE InStream createInStream() {
        return InStream{&this->inPipe, false};
    }
    PLY_INLINE OutStream createOutStream() {
        return OutStream{&this->outPipe, false};
    }
};

//------------------------------------------------------------------
// TCPListener_Winsock
//------------------------------------------------------------------
struct TCPListener_Winsock {
public:
    SOCKET listenSocket = INVALID_SOCKET;

    PLY_INLINE TCPListener_Winsock(SOCKET listenSocket = INVALID_SOCKET)
        : listenSocket{listenSocket} {
    }
    PLY_INLINE TCPListener_Winsock(TCPListener_Winsock&& other) {
        this->listenSocket = other.listenSocket;
        other.listenSocket = INVALID_SOCKET;
    }
    PLY_INLINE ~TCPListener_Winsock() {
        if (this->listenSocket >= 0) {
            closesocket(this->listenSocket);
        }
    }
    PLY_INLINE void operator=(TCPListener_Winsock&& other) {
        if (this->listenSocket >= 0) {
            closesocket(this->listenSocket);
        }
        this->listenSocket = other.listenSocket;
        other.listenSocket = INVALID_SOCKET;
    }
    PLY_INLINE bool isValid() {
        return this->listenSocket >= 0;
    }
    PLY_INLINE void endComm() {
        shutdown(this->listenSocket, SD_BOTH);
    }
    PLY_INLINE void close() {
        if (this->listenSocket >= 0) {
            closesocket(this->listenSocket);
            this->listenSocket = INVALID_SOCKET;
        }
    }

    PLY_DLL_ENTRY Owned<TCPConnection_Winsock> accept();
};

} // namespace ply

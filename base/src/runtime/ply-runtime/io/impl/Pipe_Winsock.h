/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                   ┃
┃    ╱   ╱╲    Plywood Multimedia Toolkit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/          ┃
┃    └──┴┴┴┘                                 ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/

#pragma once
#include <ply-runtime/Core.h>

#if !PLY_TARGET_WIN32
#error "File should not be included"
#endif

#include <ply-runtime/io/Pipe.h>
#include <winsock2.h>

namespace ply {

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

} // namespace ply

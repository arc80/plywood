/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                    ┃
┃    ╱   ╱╲    Plywood C++ Development Kit    ┃           
┃   ╱___╱╭╮╲   https://plywood.dev/           ┃ 
┃    └──┴┴┴┘                                  ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/

#pragma once
#include <ply-runtime/Core.h>
#include <ply-platform/Util.h>

namespace ply {

class MemPage {
public:
    struct Info {
        uptr allocationGranularity;
        uptr pageSize;
    };

    static const Info& getInfo();

    static bool alloc(char*& outAddr, uptr numBytes);
    static bool reserve(char*& outAddr, uptr numBytes);
    static void commit(char* addr, uptr numBytes);
    static void decommit(char* addr, uptr numBytes);
    // On Windows, free() is only able to free a single entire region returned by alloc() or
    // reserve(). For this reason, dlmalloc doesn't use MemPage_Win32.
    static void free(char* addr, uptr numBytes);
};

} // namespace ply

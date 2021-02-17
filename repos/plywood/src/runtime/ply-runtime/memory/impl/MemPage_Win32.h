/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-platform/Util.h>

namespace ply {

struct MemPage_Win32 {
    struct Info {
        uptr allocationGranularity;
        uptr pageSize;
    };

    static const Info& getInfo();

    static bool alloc(char*& baseAddr, uptr numBytes);
    static bool reserve(char*& baseAddr, uptr numBytes);
    static void commit(char* addr, uptr numBytes);
    static void decommit(char* addr, uptr numBytes);
    // Note that unlike MemPage_POSIX, MemPage_Win32::free() is only able to free a single entire
    // region returned by alloc() or reserve(). For this reason, Heap_DL doesn't use MemPage_Win32.
    static void free(char* baseAddr, uptr numBytes);
};

} // namespace ply

/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-platform/Util.h>

namespace ply {

class MemPage_POSIX {
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
    static void free(char* addr, uptr numBytes);
};

} // namespace ply

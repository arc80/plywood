/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-platform/Util.h>
#include <unistd.h>
#include <sys/mman.h>

namespace ply {

class MemPage_POSIX {
public:
    struct Info {
        ureg allocationGranularity;
        ureg pageSize;
    };

    static Info getInfoInternal() {
        long result = sysconf(_SC_PAGE_SIZE);
        PLY_ASSERT(isPowerOf2(result));
        return {(ureg) result, (ureg) result};
    }

    static const Info& getInfo() {
        static Info info = getInfoInternal();
        return info;
    }

    static bool alloc(void*& result, ureg size, bool topDownHint = false) {
        result = mmap(0, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        PLY_ASSERT(result != MAP_FAILED);
        return true;
    }

    static bool free(void* ptr, ureg size) {
        munmap(ptr, size);
        return true;
    }
};

} // namespace ply

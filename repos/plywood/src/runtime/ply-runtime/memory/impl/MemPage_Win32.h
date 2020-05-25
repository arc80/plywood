/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-platform/Util.h>

namespace ply {

class MemPage_Win32 {
public:
    struct Info {
        ureg allocationGranularity;
        ureg pageSize;
    };

    static Info getInfoInternal() {
        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);
        PLY_ASSERT(isPowerOf2(sysInfo.dwAllocationGranularity));
        PLY_ASSERT(isPowerOf2(sysInfo.dwPageSize));
        return {sysInfo.dwAllocationGranularity, sysInfo.dwPageSize};
    }

    static const Info& getInfo() {
        static Info info = getInfoInternal();
        return info;
    }

    static bool alloc(void*& result, ureg size, bool topDownHint = false) {
        DWORD type = MEM_RESERVE | MEM_COMMIT;
        if (topDownHint)
            type |= MEM_TOP_DOWN;
        result = VirtualAlloc(0, (SIZE_T) size, type, PAGE_READWRITE);
        return (result != NULL);
    }

    static bool free(void* ptr, ureg size) {
        MEMORY_BASIC_INFORMATION minfo;
        while (sreg(size) > 0) {
            if (VirtualQuery((LPCVOID) ptr, &minfo, sizeof(minfo)) == 0)
                return false;
            if (minfo.BaseAddress != ptr || minfo.AllocationBase != ptr ||
                minfo.State != MEM_COMMIT || minfo.RegionSize > size)
                return false;
            if (VirtualFree(ptr, 0, MEM_RELEASE) == 0)
                return false;
            ptr = PLY_PTR_OFFSET(ptr, minfo.RegionSize);
            size -= minfo.RegionSize;
        }
        return true;
    }
};

} // namespace ply

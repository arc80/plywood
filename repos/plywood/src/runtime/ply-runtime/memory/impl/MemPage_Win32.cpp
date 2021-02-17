/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/

#include <ply-runtime/Precomp.h>

#if PLY_TARGET_WIN32

#include <ply-runtime/memory/impl/MemPage_Win32.h>

namespace ply {

const MemPage_Win32::Info& MemPage_Win32::getInfo() {
    static Info info = []() -> Info {
        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);
        PLY_ASSERT(isPowerOf2(sysInfo.dwAllocationGranularity));
        PLY_ASSERT(isPowerOf2(sysInfo.dwPageSize));
        return {sysInfo.dwAllocationGranularity, sysInfo.dwPageSize};
    }();
    return info;
}

bool MemPage_Win32::alloc(char*& outAddr, uptr numBytes) {
    PLY_ASSERT(isAlignedPowerOf2(numBytes, getInfo().allocationGranularity));

    DWORD type = MEM_RESERVE | MEM_COMMIT;
    outAddr = (char*) VirtualAlloc(0, (SIZE_T) numBytes, type, PAGE_READWRITE);
    return (outAddr != NULL);
}

bool MemPage_Win32::reserve(char*& outAddr, uptr numBytes) {
    PLY_ASSERT(isAlignedPowerOf2(numBytes, getInfo().allocationGranularity));

    DWORD type = MEM_RESERVE;
    outAddr = (char*) VirtualAlloc(0, (SIZE_T) numBytes, type, PAGE_READWRITE);
    return (outAddr != NULL);
}

void MemPage_Win32::commit(char* addr, uptr numBytes) {
    PLY_ASSERT(isAlignedPowerOf2((uptr) addr, getInfo().pageSize));
    PLY_ASSERT(isAlignedPowerOf2(numBytes, getInfo().pageSize));

    DWORD type = MEM_COMMIT;
    LPVOID result = VirtualAlloc(addr, (SIZE_T) numBytes, type, PAGE_READWRITE);
    PLY_ASSERT(result != NULL);
    PLY_UNUSED(result);
}

void MemPage_Win32::decommit(char* addr, uptr numBytes) {
    PLY_ASSERT(isAlignedPowerOf2((uptr) addr, getInfo().pageSize));
    PLY_ASSERT(isAlignedPowerOf2(numBytes, getInfo().pageSize));

    DWORD type = MEM_COMMIT;
    LPVOID result = VirtualAlloc(addr, (SIZE_T) numBytes, type, PAGE_READWRITE);
    PLY_ASSERT(result != NULL);
    PLY_UNUSED(result);
}

void MemPage_Win32::free(char* addr, uptr numBytes) {
    PLY_ASSERT(isAlignedPowerOf2((uptr) addr, getInfo().allocationGranularity));
    PLY_ASSERT(isAlignedPowerOf2(numBytes, getInfo().allocationGranularity));

#if PLY_WITH_ASSERTS
    {
        // Must be entire reserved address space range
        MEMORY_BASIC_INFORMATION memInfo;
        SIZE_T rc = VirtualQuery(addr, &memInfo, sizeof(memInfo));
        PLY_ASSERT(rc != 0);
        PLY_UNUSED(rc);
        PLY_ASSERT(memInfo.BaseAddress == addr);
        PLY_ASSERT(memInfo.AllocationBase == addr);
        PLY_ASSERT(memInfo.RegionSize <= numBytes);
    }
#endif
    BOOL rc2 = VirtualFree(addr, 0, MEM_RELEASE);
    PLY_ASSERT(rc2);
    PLY_UNUSED(rc2);
}

} // namespace ply

#endif // PLY_TARGET_WIN32

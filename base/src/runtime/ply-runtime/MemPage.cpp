/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                    ┃
┃    ╱   ╱╲    Plywood C++ Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/           ┃
┃    └──┴┴┴┘                                  ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/

#include <ply-runtime/Precomp.h>
#include <ply-runtime/MemPage.h>

#if PLY_TARGET_WIN32

//  ▄▄    ▄▄ ▄▄            ▄▄
//  ██ ▄▄ ██ ▄▄ ▄▄▄▄▄   ▄▄▄██  ▄▄▄▄  ▄▄    ▄▄  ▄▄▄▄
//  ▀█▄██▄█▀ ██ ██  ██ ██  ██ ██  ██ ██ ██ ██ ▀█▄▄▄
//   ██▀▀██  ██ ██  ██ ▀█▄▄██ ▀█▄▄█▀  ██▀▀██   ▄▄▄█▀
//

namespace ply {

const MemPage::Info& MemPage::getInfo() {
    static Info info = []() -> Info {
        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);
        PLY_ASSERT(isPowerOf2(sysInfo.dwAllocationGranularity));
        PLY_ASSERT(isPowerOf2(sysInfo.dwPageSize));
        return {sysInfo.dwAllocationGranularity, sysInfo.dwPageSize};
    }();
    return info;
}

bool MemPage::alloc(char*& outAddr, uptr numBytes) {
    PLY_ASSERT(isAlignedPowerOf2(numBytes, getInfo().allocationGranularity));

    DWORD type = MEM_RESERVE | MEM_COMMIT;
    outAddr = (char*) VirtualAlloc(0, (SIZE_T) numBytes, type, PAGE_READWRITE);
    return (outAddr != NULL);
}

bool MemPage::reserve(char*& outAddr, uptr numBytes) {
    PLY_ASSERT(isAlignedPowerOf2(numBytes, getInfo().allocationGranularity));

    DWORD type = MEM_RESERVE;
    outAddr = (char*) VirtualAlloc(0, (SIZE_T) numBytes, type, PAGE_READWRITE);
    return (outAddr != NULL);
}

void MemPage::commit(char* addr, uptr numBytes) {
    PLY_ASSERT(isAlignedPowerOf2((uptr) addr, getInfo().pageSize));
    PLY_ASSERT(isAlignedPowerOf2(numBytes, getInfo().pageSize));

    DWORD type = MEM_COMMIT;
    LPVOID result = VirtualAlloc(addr, (SIZE_T) numBytes, type, PAGE_READWRITE);
    PLY_ASSERT(result != NULL);
    PLY_UNUSED(result);
}

void MemPage::decommit(char* addr, uptr numBytes) {
    PLY_ASSERT(isAlignedPowerOf2((uptr) addr, getInfo().pageSize));
    PLY_ASSERT(isAlignedPowerOf2(numBytes, getInfo().pageSize));

    DWORD type = MEM_COMMIT;
    LPVOID result = VirtualAlloc(addr, (SIZE_T) numBytes, type, PAGE_READWRITE);
    PLY_ASSERT(result != NULL);
    PLY_UNUSED(result);
}

void MemPage::free(char* addr, uptr numBytes) {
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

#elif PLY_TARGET_POSIX

//  ▄▄▄▄▄   ▄▄▄▄   ▄▄▄▄  ▄▄▄▄ ▄▄  ▄▄
//  ██  ██ ██  ██ ██  ▀▀  ██  ▀█▄▄█▀
//  ██▀▀▀  ██  ██  ▀▀▀█▄  ██   ▄██▄
//  ██     ▀█▄▄█▀ ▀█▄▄█▀ ▄██▄ ██  ██
//

#include <unistd.h>
#include <sys/mman.h>

namespace ply {

const MemPage::Info& MemPage::getInfo() {
    static Info info = []() -> Info {
        long result = sysconf(_SC_PAGE_SIZE);
        PLY_ASSERT(isPowerOf2(result));
        return {(ureg) result, (ureg) result};
    }();
    return info;
}

bool MemPage::alloc(char*& outAddr, uptr numBytes) {
    PLY_ASSERT(isAlignedPowerOf2(numBytes, getInfo().allocationGranularity));

    outAddr = (char*) mmap(0, numBytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    PLY_ASSERT(outAddr != MAP_FAILED);
    return true;
}

bool MemPage::reserve(char*& outAddr, uptr numBytes) {
    PLY_ASSERT(isAlignedPowerOf2(numBytes, getInfo().allocationGranularity));

    outAddr = (char*) mmap(0, numBytes, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    PLY_ASSERT(outAddr != MAP_FAILED);
    return true;
}

void MemPage::commit(char* addr, uptr numBytes) {
    PLY_ASSERT(isAlignedPowerOf2((uptr) addr, getInfo().pageSize));
    PLY_ASSERT(isAlignedPowerOf2(numBytes, getInfo().pageSize));

    int rc = mprotect(addr, numBytes, PROT_READ | PROT_WRITE);
    PLY_ASSERT(rc == 0);
    PLY_UNUSED(rc);
}

void MemPage::decommit(char* addr, uptr numBytes) {
    PLY_ASSERT(isAlignedPowerOf2((uptr) addr, getInfo().pageSize));
    PLY_ASSERT(isAlignedPowerOf2(numBytes, getInfo().pageSize));

    int rc = madvise(addr, numBytes, MADV_DONTNEED);
    PLY_ASSERT(rc == 0);
    rc = mprotect(addr, numBytes, PROT_NONE);
    PLY_ASSERT(rc == 0);
    PLY_UNUSED(rc);
}

void MemPage::free(char* addr, uptr numBytes) {
    PLY_ASSERT(isAlignedPowerOf2((uptr) addr, getInfo().allocationGranularity));
    PLY_ASSERT(isAlignedPowerOf2(numBytes, getInfo().allocationGranularity));

    munmap(addr, numBytes);
}

} // namespace ply

#else
#error "Unsupported target!"
#endif

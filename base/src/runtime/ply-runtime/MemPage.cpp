/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-runtime.h>

#if PLY_TARGET_WIN32

//  ▄▄    ▄▄ ▄▄            ▄▄
//  ██ ▄▄ ██ ▄▄ ▄▄▄▄▄   ▄▄▄██  ▄▄▄▄  ▄▄    ▄▄  ▄▄▄▄
//  ▀█▄██▄█▀ ██ ██  ██ ██  ██ ██  ██ ██ ██ ██ ▀█▄▄▄
//   ██▀▀██  ██ ██  ██ ▀█▄▄██ ▀█▄▄█▀  ██▀▀██   ▄▄▄█▀
//

namespace ply {

const MemPage::Info& MemPage::get_info() {
    static Info info = []() -> Info {
        SYSTEM_INFO sys_info;
        GetSystemInfo(&sys_info);
        PLY_ASSERT(is_power_of2(sys_info.dwAllocationGranularity));
        PLY_ASSERT(is_power_of2(sys_info.dwPageSize));
        return {sys_info.dwAllocationGranularity, sys_info.dwPageSize};
    }();
    return info;
}

bool MemPage::alloc(char*& out_addr, uptr num_bytes) {
    PLY_ASSERT(is_aligned_power_of2(num_bytes, get_info().allocation_granularity));

    DWORD type = MEM_RESERVE | MEM_COMMIT;
    out_addr = (char*) VirtualAlloc(0, (SIZE_T) num_bytes, type, PAGE_READWRITE);
    return (out_addr != NULL);
}

bool MemPage::reserve(char*& out_addr, uptr num_bytes) {
    PLY_ASSERT(is_aligned_power_of2(num_bytes, get_info().allocation_granularity));

    DWORD type = MEM_RESERVE;
    out_addr = (char*) VirtualAlloc(0, (SIZE_T) num_bytes, type, PAGE_READWRITE);
    return (out_addr != NULL);
}

void MemPage::commit(char* addr, uptr num_bytes) {
    PLY_ASSERT(is_aligned_power_of2((uptr) addr, get_info().page_size));
    PLY_ASSERT(is_aligned_power_of2(num_bytes, get_info().page_size));

    DWORD type = MEM_COMMIT;
    LPVOID result = VirtualAlloc(addr, (SIZE_T) num_bytes, type, PAGE_READWRITE);
    PLY_ASSERT(result != NULL);
    PLY_UNUSED(result);
}

void MemPage::decommit(char* addr, uptr num_bytes) {
    PLY_ASSERT(is_aligned_power_of2((uptr) addr, get_info().page_size));
    PLY_ASSERT(is_aligned_power_of2(num_bytes, get_info().page_size));

    DWORD type = MEM_COMMIT;
    LPVOID result = VirtualAlloc(addr, (SIZE_T) num_bytes, type, PAGE_READWRITE);
    PLY_ASSERT(result != NULL);
    PLY_UNUSED(result);
}

void MemPage::free(char* addr, uptr num_bytes) {
    PLY_ASSERT(is_aligned_power_of2((uptr) addr, get_info().allocation_granularity));
    PLY_ASSERT(is_aligned_power_of2(num_bytes, get_info().allocation_granularity));

#if PLY_WITH_ASSERTS
    {
        // Must be entire reserved address space range
        MEMORY_BASIC_INFORMATION mem_info;
        SIZE_T rc = VirtualQuery(addr, &mem_info, sizeof(mem_info));
        PLY_ASSERT(rc != 0);
        PLY_UNUSED(rc);
        PLY_ASSERT(mem_info.BaseAddress == addr);
        PLY_ASSERT(mem_info.AllocationBase == addr);
        PLY_ASSERT(mem_info.RegionSize <= num_bytes);
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

const MemPage::Info& MemPage::get_info() {
    static Info info = []() -> Info {
        long result = sysconf(_SC_PAGE_SIZE);
        PLY_ASSERT(is_power_of2(result));
        return {(ureg) result, (ureg) result};
    }();
    return info;
}

bool MemPage::alloc(char*& out_addr, uptr num_bytes) {
    PLY_ASSERT(is_aligned_power_of2(num_bytes, get_info().allocation_granularity));

    out_addr = (char*) mmap(0, num_bytes, PROT_READ | PROT_WRITE,
                            MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    PLY_ASSERT(out_addr != MAP_FAILED);
    return true;
}

bool MemPage::reserve(char*& out_addr, uptr num_bytes) {
    PLY_ASSERT(is_aligned_power_of2(num_bytes, get_info().allocation_granularity));

    out_addr =
        (char*) mmap(0, num_bytes, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    PLY_ASSERT(out_addr != MAP_FAILED);
    return true;
}

void MemPage::commit(char* addr, uptr num_bytes) {
    PLY_ASSERT(is_aligned_power_of2((uptr) addr, get_info().page_size));
    PLY_ASSERT(is_aligned_power_of2(num_bytes, get_info().page_size));

    int rc = mprotect(addr, num_bytes, PROT_READ | PROT_WRITE);
    PLY_ASSERT(rc == 0);
    PLY_UNUSED(rc);
}

void MemPage::decommit(char* addr, uptr num_bytes) {
    PLY_ASSERT(is_aligned_power_of2((uptr) addr, get_info().page_size));
    PLY_ASSERT(is_aligned_power_of2(num_bytes, get_info().page_size));

    int rc = madvise(addr, num_bytes, MADV_DONTNEED);
    PLY_ASSERT(rc == 0);
    rc = mprotect(addr, num_bytes, PROT_NONE);
    PLY_ASSERT(rc == 0);
    PLY_UNUSED(rc);
}

void MemPage::free(char* addr, uptr num_bytes) {
    PLY_ASSERT(is_aligned_power_of2((uptr) addr, get_info().allocation_granularity));
    PLY_ASSERT(is_aligned_power_of2(num_bytes, get_info().allocation_granularity));

    munmap(addr, num_bytes);
}

} // namespace ply

#else
#error "Unsupported target!"
#endif

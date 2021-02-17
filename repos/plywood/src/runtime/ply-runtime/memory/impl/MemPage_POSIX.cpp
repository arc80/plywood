/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/

#include <ply-runtime/Precomp.h>

#if PLY_TARGET_POSIX

#include <ply-runtime/memory/impl/MemPage_POSIX.h>
#include <unistd.h>
#include <sys/mman.h>

namespace ply {

const MemPage_POSIX::Info& MemPage_POSIX::getInfo() {
    static Info info = []() -> Info {
        long result = sysconf(_SC_PAGE_SIZE);
        PLY_ASSERT(isPowerOf2(result));
        return {(ureg) result, (ureg) result};
    }();
    return info;
}

bool MemPage_POSIX::alloc(char*& outAddr, uptr numBytes) {
    PLY_ASSERT(isAlignedPowerOf2(numBytes, getInfo().allocationGranularity));

    outAddr = (char*) mmap(0, numBytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    PLY_ASSERT(outAddr != MAP_FAILED);
    return true;
}

bool MemPage_POSIX::reserve(char*& outAddr, uptr numBytes) {
    PLY_ASSERT(isAlignedPowerOf2(numBytes, getInfo().allocationGranularity));

#if PLY_KERNEL_LINUX
    outAddr = (char*) mmap(0, numBytes, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    PLY_ASSERT(outAddr != MAP_FAILED);
#else
    PLY_FORCE_CRASH(); // Not supported yet
#endif
    return true;
}

void MemPage_POSIX::commit(char* addr, uptr numBytes) {
    PLY_ASSERT(isAlignedPowerOf2((uptr) addr, getInfo().pageSize));
    PLY_ASSERT(isAlignedPowerOf2(numBytes, getInfo().pageSize));

#if PLY_KERNEL_LINUX
    int rc = mprotect(addr, numBytes, PROT_READ | PROT_WRITE);
    PLY_ASSERT(rc == 0);
    PLY_UNUSED(rc);
#else
    PLY_FORCE_CRASH(); // Not supported yet
#endif
}

void MemPage_POSIX::decommit(char* addr, uptr numBytes) {
    PLY_ASSERT(isAlignedPowerOf2((uptr) addr, getInfo().pageSize));
    PLY_ASSERT(isAlignedPowerOf2(numBytes, getInfo().pageSize));

#if PLY_KERNEL_LINUX
    int rc = madvise(addr, numBytes, MADV_DONTNEED);
    PLY_ASSERT(rc == 0);
    rc = mprotect(addr, numBytes, PROT_NONE);
    PLY_ASSERT(rc == 0);
    PLY_UNUSED(rc);
#else
    PLY_FORCE_CRASH(); // Not supported yet
#endif
}

void MemPage_POSIX::free(char* addr, uptr numBytes) {
    PLY_ASSERT(isAlignedPowerOf2((uptr) addr, getInfo().allocationGranularity));
    PLY_ASSERT(isAlignedPowerOf2(numBytes, getInfo().allocationGranularity));

    munmap(addr, numBytes);
}

} // namespace ply

#endif // PLY_TARGET_POSIX

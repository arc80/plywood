/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-runtime.h>

namespace ply {

PLY_NO_INLINE BaseBigPool::BaseBigPool(uptr numReservedBytes) {
    uptr allocationGranularity = MemPage::getInfo().allocationGranularity;
    this->numReservedBytes = alignPowerOf2(numReservedBytes, allocationGranularity);
    bool rc = MemPage::reserve(this->base, this->numReservedBytes);
    PLY_ASSERT(rc);
    PLY_UNUSED(rc);
}

PLY_NO_INLINE BaseBigPool::~BaseBigPool() {
    MemPage::free(this->base, this->numReservedBytes);
}

PLY_NO_INLINE void BaseBigPool::commitPages(uptr newTotalBytes) {
    uptr allocationGranularity = MemPage::getInfo().allocationGranularity;
    PLY_ASSERT(isAlignedPowerOf2(this->numCommittedBytes, allocationGranularity));
    uptr newPageBoundary = alignPowerOf2(newTotalBytes, allocationGranularity);
    PLY_ASSERT(newPageBoundary > this->numCommittedBytes);
    MemPage::commit(this->base + this->numCommittedBytes,
                    newPageBoundary - this->numCommittedBytes);
    this->numCommittedBytes = newPageBoundary;
}

} // namespace ply

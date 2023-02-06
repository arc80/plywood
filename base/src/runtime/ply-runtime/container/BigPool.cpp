/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-runtime.h>

namespace ply {

PLY_NO_INLINE BaseBigPool::BaseBigPool(uptr num_reserved_bytes) {
    uptr allocation_granularity = MemPage::get_info().allocation_granularity;
    this->num_reserved_bytes =
        align_power_of2(num_reserved_bytes, allocation_granularity);
    bool rc = MemPage::reserve(this->base, this->num_reserved_bytes);
    PLY_ASSERT(rc);
    PLY_UNUSED(rc);
}

PLY_NO_INLINE BaseBigPool::~BaseBigPool() {
    MemPage::free(this->base, this->num_reserved_bytes);
}

PLY_NO_INLINE void BaseBigPool::commit_pages(uptr new_total_bytes) {
    uptr allocation_granularity = MemPage::get_info().allocation_granularity;
    PLY_ASSERT(is_aligned_power_of2(this->num_committed_bytes, allocation_granularity));
    uptr new_page_boundary = align_power_of2(new_total_bytes, allocation_granularity);
    PLY_ASSERT(new_page_boundary > this->num_committed_bytes);
    MemPage::commit(this->base + this->num_committed_bytes,
                    new_page_boundary - this->num_committed_bytes);
    this->num_committed_bytes = new_page_boundary;
}

} // namespace ply

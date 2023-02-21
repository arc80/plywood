/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-runtime/Precomp.h>
#include <ply-runtime/Core.h>

namespace ply {

LabelStorage g_labelStorage;

LabelStorage::LabelStorage() {
    this->big_pool.append('\0');
    this->big_pool.append('\0');
}

Label LabelStorage::insert(StringView view) {
    PLY_RACE_DETECT_GUARD(this->race_detector);

    bool was_found = false;
    u32* idx = this->str_to_index.insert_or_find(view, &was_found);
    if (was_found)
        return Label{*idx};

    u32 num_entry_bytes =
        align_power_of2(view.num_bytes + (view.num_bytes < 0x8000 ? 3 : 5), 2);
    u32 result_id = check_cast<u32>(this->big_pool.num_items());
    *idx = result_id;

    char* ptr = this->big_pool.alloc(num_entry_bytes);
    char* end = ptr + num_entry_bytes;
    if (view.num_bytes < 0x8000) {
        *(u16*) ptr = (u16) view.num_bytes;
        ptr += 2;
    } else {
        *(u16*) ptr = (u16) (view.num_bytes | 0x8000);
        ((u16*) ptr)[1] = (u16) (view.num_bytes >> 15);
        ptr += 4;
    }
    memcpy(ptr, view.bytes, view.num_bytes);
    ptr += view.num_bytes;
    while (ptr < end) {
        *ptr++ = 0;
    }
    return Label{result_id};
}

Label LabelStorage::find(StringView view) const {
    PLY_RACE_DETECT_GUARD(this->race_detector);

    if (const u32* idx = this->str_to_index.find(view))
        return Label{*idx};
    return {};
}

StringView LabelStorage::view(Label label) const {
    PLY_RACE_DETECT_GUARD(this->race_detector);

    const char* ptr = this->big_pool.get(label.idx);
    u32 num_bytes = *(u16*) ptr;
    ptr += 2;
    if (num_bytes >= 0x8000) {
        num_bytes = num_bytes & 0x7fff;
        num_bytes |= u32(*(u16*) ptr) << 15;
        ptr += 2;
    }
    return {ptr, num_bytes};
}

} // namespace ply

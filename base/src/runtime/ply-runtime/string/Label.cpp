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

PLY_NO_INLINE LabelStorage::LabelStorage() {
    this->big_pool.append('\0');
}

PLY_NO_INLINE Label LabelStorage::insert(StringView view) {
    PLY_RACE_DETECT_GUARD(this->race_detector);

    auto cursor = this->str_to_index.insert_or_find(view, &this->big_pool);
    if (cursor.was_found())
        return Label{*cursor};

    u32 num_entry_bytes =
        impl::LabelEncoder::get_enc_len(view.num_bytes) + view.num_bytes;
    u32 result_id = check_cast<u32>(this->big_pool.num_items());
    *cursor = result_id;

    char* ptr = this->big_pool.alloc(num_entry_bytes);
    impl::LabelEncoder::encode_value(ptr, view.num_bytes);
    memcpy(ptr, view.bytes, view.num_bytes);
    PLY_ASSERT(ptr + view.num_bytes == this->big_pool.end());
    return Label{result_id};
}

PLY_NO_INLINE Label LabelStorage::find(StringView view) const {
    PLY_RACE_DETECT_GUARD(this->race_detector);

    auto cursor = this->str_to_index.find(view, &this->big_pool);
    if (cursor.was_found())
        return Label{*cursor};
    return {};
}

PLY_NO_INLINE StringView LabelStorage::view(Label label) const {
    PLY_RACE_DETECT_GUARD(this->race_detector);

    const char* ptr = this->big_pool.get(label.idx);
    u32 num_bytes = impl::LabelEncoder::decode_value(ptr);
    return {ptr, num_bytes};
}

} // namespace ply

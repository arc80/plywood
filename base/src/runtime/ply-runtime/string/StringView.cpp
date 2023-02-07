/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-runtime.h>

namespace ply {

u32 StringView::num_codepoints(UnicodeType decoder_type) const {
    Unicode decoder{decoder_type};
    u32 num_codepoints = 0;
    ViewInStream in{*this};
    while (Unicode{UTF8}.decode_point(in) >= 0) {
        num_codepoints++;
    }
    return num_codepoints;
}

bool StringView::starts_with(StringView other) const {
    if (other.num_bytes > num_bytes)
        return false;
    return memcmp(bytes, other.bytes, other.num_bytes) == 0;
}

bool StringView::ends_with(StringView other) const {
    if (other.num_bytes > num_bytes)
        return false;
    return memcmp(bytes + num_bytes - other.num_bytes, other.bytes, other.num_bytes) ==
           0;
}

StringView StringView::trim(bool (*match_func)(char), bool left, bool right) const {
    const char* start = this->bytes;
    const char* end = start + this->num_bytes;
    if (left) {
        while ((start < end) && match_func(*start)) {
            start++;
        }
    }
    if (right) {
        while ((start < end) && match_func(end[-1])) {
            end--;
        }
    }
    return StringView::from_range(start, end);
}

Array<StringView> StringView::split_byte(char sep) const {
    Array<StringView> result;
    const char* cur = this->bytes;
    const char* end = this->bytes + this->num_bytes;
    const char* split_start = nullptr;
    while (cur < end) {
        if (*cur == sep) {
            if (split_start) {
                result.append(StringView::from_range(split_start, cur));
                split_start = nullptr;
            }
        } else {
            if (!split_start) {
                split_start = cur;
            }
        }
        cur++;
    }
    if (split_start) {
        result.append(StringView::from_range(split_start, cur));
    }
    if (result.is_empty()) {
        result.append({this->bytes, 0});
    }
    return result;
}

String StringView::upper_asc() const {
    String result = String::allocate(this->num_bytes);
    for (u32 i = 0; i < this->num_bytes; i++) {
        char c = this->bytes[i];
        if (c >= 'a' && c <= 'z') {
            c += 'A' - 'a';
        }
        result.bytes[i] = c;
    }
    return result;
}

String StringView::lower_asc() const {
    String result = String::allocate(this->num_bytes);
    for (u32 i = 0; i < this->num_bytes; i++) {
        char c = this->bytes[i];
        if (c >= 'A' && c <= 'Z') {
            c += 'a' - 'A';
        }
        result.bytes[i] = c;
    }
    return result;
}

String StringView::reversed_bytes() const {
    String result = String::allocate(this->num_bytes);
    const char* src = this->bytes + this->num_bytes;
    for (u32 i = 0; i < this->num_bytes; i++) {
        src--;
        result.bytes[i] = *src;
    }
    return result;
}

String StringView::filter_bytes(char (*filter_func)(char)) const {
    String result = String::allocate(this->num_bytes);
    for (u32 i = 0; i < this->num_bytes; i++) {
        result.bytes[i] = filter_func(this->bytes[i]);
    }
    return result;
}

String StringView::join(ArrayView<const StringView> comps) const {
    MemOutStream mout;
    bool first = true;
    for (StringView comp : comps) {
        if (!first) {
            mout << *this;
        }
        mout << comp;
        first = false;
    }
    return mout.move_to_string();
}

HybridString StringView::with_null_terminator() const {
    if (this->includes_null_terminator()) {
        return *this;
    }
    String result = String::allocate(this->num_bytes + 1);
    memcpy(result.bytes, this->bytes, this->num_bytes);
    result.bytes[this->num_bytes] = 0;
    return result;
}

StringView StringView::without_null_terminator() const {
    if (!this->includes_null_terminator()) {
        return *this;
    }
    return {this->bytes, this->num_bytes - 1};
}

s32 compare(StringView a, StringView b) {
    u32 compare_bytes = min(a.num_bytes, b.num_bytes);
    const u8* u0 = (const u8*) a.bytes;
    const u8* u1 = (const u8*) b.bytes;
    const u8* u_end0 = u0 + compare_bytes;
    while (u0 < u_end0) {
        s32 diff = *u0 - *u1;
        if (diff != 0)
            return diff;
        u0++;
        u1++;
    }
    return a.num_bytes - b.num_bytes;
}

String operator+(StringView a, StringView b) {
    String result = String::allocate(a.num_bytes + b.num_bytes);
    memcpy(result.bytes, a.bytes, a.num_bytes);
    memcpy(result.bytes + a.num_bytes, b.bytes, b.num_bytes);
    return result;
}

String operator*(StringView str, u32 count) {
    String result = String::allocate(str.num_bytes * count);
    char* dst = result.bytes;
    for (u32 i = 0; i < count; i++) {
        memcpy(dst, str.bytes, str.num_bytes);
        dst += str.num_bytes;
    }
    return result;
}

} // namespace ply

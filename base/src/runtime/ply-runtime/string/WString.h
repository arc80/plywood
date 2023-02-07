/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once
#include <ply-runtime.h>
#include <string>

namespace ply {

//-----------------------------------------------------------------------
// WStringView
//-----------------------------------------------------------------------
struct WStringView {
    const char16_t* units = nullptr;
    u32 num_units = 0;

    WStringView() = default;
    WStringView(const char16_t* units, u32 num_units)
        : units{units}, num_units{num_units} {
    }
    StringView raw_bytes() const {
        return {(const char*) this->units, this->num_units << 1};
    }
#if PLY_TARGET_WIN32
    WStringView(LPCWSTR units)
        : units{(const char16_t*) units}, num_units{
                                              (u32) std::char_traits<char16_t>::length(
                                                  (const char16_t*) units)} {
    }
    WStringView(LPCWSTR units, u32 num_units)
        : units{(const char16_t*) units}, num_units{num_units} {
    }
#endif
};

//-----------------------------------------------------------------------
// WString
//-----------------------------------------------------------------------
struct WString {
    using View = WStringView;
    char16_t* units = nullptr;
    u32 num_units = 0;

    WString() = default;
    WString(WString&& other) : units{other.units}, num_units{other.num_units} {
        other.units = nullptr;
        other.num_units = 0;
    }
    ~WString() {
        if (units) {
            Heap.free(units);
        }
    }
    void operator=(WString&& other) {
        this->~WString();
        new (this) WString{std::move(other)};
    }
    static WString move_from_string(String&& other) {
        PLY_ASSERT(is_aligned_power_of2(uptr(other.bytes), 2));
        PLY_ASSERT(is_aligned_power_of2(other.num_bytes, 2));
        WString result;
        result.units = (char16_t*) other.bytes;
        result.num_units = other.num_bytes >> 1;
        other.bytes = nullptr;
        other.num_bytes = 0;
        return result;
    }

    bool includes_null_terminator() const {
        return this->num_units > 0 && this->units[this->num_units - 1] == 0;
    }
    static WString allocate(u32 num_units) {
        WString result;
        result.units = (char16_t*) Heap.alloc(num_units << 1);
        result.num_units = num_units;
        return result;
    }

#if PLY_TARGET_WIN32
    operator LPWSTR() const {
        PLY_ASSERT(this->includes_null_terminator()); // must be null terminated
        return (LPWSTR) this->units;
    }
#endif
};

WString to_wstring(StringView str);
String from_wstring(WStringView str);

} // namespace ply

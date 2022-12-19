/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/string/String.h>
#include <string>

namespace ply {

//-----------------------------------------------------------------------
// WStringView
//-----------------------------------------------------------------------
struct WStringView {
    const char16_t* units = nullptr;
    u32 numUnits = 0;

    PLY_INLINE WStringView() = default;
    PLY_INLINE WStringView(const char16_t* units, u32 numUnits) : units{units}, numUnits{numUnits} {
    }
    PLY_INLINE StringView stringView() const {
        return {(const char*) this->units, this->numUnits << 1};
    }
#if PLY_TARGET_WIN32
    WStringView(LPCWSTR units)
        : units{(const char16_t*) units}, numUnits{(u32) std::char_traits<char16_t>::length(
                                              (const char16_t*) units)} {
    }
    WStringView(LPCWSTR units, u32 numUnits) : units{(const char16_t*) units}, numUnits{numUnits} {
    }
#endif
};

//-----------------------------------------------------------------------
// WString
//-----------------------------------------------------------------------
struct WString {
    using View = WStringView;
    char16_t* units = nullptr;
    u32 numUnits = 0;

    PLY_INLINE WString() = default;
    PLY_INLINE WString(WString&& other) : units{other.units}, numUnits{other.numUnits} {
        other.units = nullptr;
        other.numUnits = 0;
    }
    PLY_INLINE ~WString() {
        if (units) {
            Heap.free(units);
        }
    }
    PLY_INLINE void operator=(WString&& other) {
        this->~WString();
        new (this) WString{std::move(other)};
    }
    PLY_INLINE static WString moveFromString(String&& other) {
        PLY_ASSERT(isAlignedPowerOf2(uptr(other.bytes), 2));
        PLY_ASSERT(isAlignedPowerOf2(other.numBytes, 2));
        WString result;
        result.units = (char16_t*) other.bytes;
        result.numUnits = other.numBytes >> 1;
        other.bytes = nullptr;
        other.numBytes = 0;
        return result;
    }

    PLY_INLINE bool includesNullTerminator() const {
        return this->numUnits > 0 && this->units[this->numUnits - 1] == 0;
    }
    PLY_INLINE static WString allocate(u32 numUnits) {
        WString result;
        result.units = (char16_t*) Heap.alloc(numUnits << 1);
        result.numUnits = numUnits;
        return result;
    }

#if PLY_TARGET_WIN32
    operator LPWSTR() const {
        PLY_ASSERT(this->includesNullTerminator()); // must be null terminated
        return (LPWSTR) this->units;
    }
#endif
};

} // namespace ply

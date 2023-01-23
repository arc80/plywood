/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-runtime/Precomp.h>
#include <ply-runtime/string/String.h>
#include <ply-runtime/string/TextEncoding.h>
#include <ply-runtime/io/OutStream.h>
#include <ply-runtime/container/Array.h>

namespace ply {

PLY_NO_INLINE bool StringView::startsWith(StringView other) const {
    if (other.numBytes > numBytes)
        return false;
    return memcmp(bytes, other.bytes, other.numBytes) == 0;
}

PLY_NO_INLINE bool StringView::endsWith(StringView other) const {
    if (other.numBytes > numBytes)
        return false;
    return memcmp(bytes + numBytes - other.numBytes, other.bytes, other.numBytes) == 0;
}

PLY_NO_INLINE StringView StringView::trim(bool (*matchFunc)(char), bool left, bool right) const {
    const char* start = this->bytes;
    const char* end = start + this->numBytes;
    if (left) {
        while ((start < end) && matchFunc(*start)) {
            start++;
        }
    }
    if (right) {
        while ((start < end) && matchFunc(end[-1])) {
            end--;
        }
    }
    return StringView::fromRange(start, end);
}

PLY_NO_INLINE Array<StringView> StringView::splitByte(char sep) const {
    Array<StringView> result;
    const char* cur = this->bytes;
    const char* end = this->bytes + this->numBytes;
    const char* splitStart = nullptr;
    while (cur < end) {
        if (*cur == sep) {
            if (splitStart) {
                result.append(StringView::fromRange(splitStart, cur));
                splitStart = nullptr;
            }
        } else {
            if (!splitStart) {
                splitStart = cur;
            }
        }
        cur++;
    }
    if (splitStart) {
        result.append(StringView::fromRange(splitStart, cur));
    }
    if (result.isEmpty()) {
        result.append({this->bytes, 0});
    }
    return result;
}

PLY_NO_INLINE String StringView::upperAsc() const {
    String result = String::allocate(this->numBytes);
    for (u32 i = 0; i < this->numBytes; i++) {
        char c = this->bytes[i];
        if (c >= 'a' && c <= 'z') {
            c += 'A' - 'a';
        }
        result.bytes[i] = c;
    }
    return result;
}

PLY_NO_INLINE String StringView::lowerAsc() const {
    String result = String::allocate(this->numBytes);
    for (u32 i = 0; i < this->numBytes; i++) {
        char c = this->bytes[i];
        if (c >= 'A' && c <= 'Z') {
            c += 'a' - 'A';
        }
        result.bytes[i] = c;
    }
    return result;
}

PLY_NO_INLINE String StringView::reversedBytes() const {
    String result = String::allocate(this->numBytes);
    const char* src = this->bytes + this->numBytes;
    for (u32 i = 0; i < this->numBytes; i++) {
        src--;
        result.bytes[i] = *src;
    }
    return result;
}

PLY_NO_INLINE String StringView::filterBytes(char (*filterFunc)(char)) const {
    String result = String::allocate(this->numBytes);
    for (u32 i = 0; i < this->numBytes; i++) {
        result.bytes[i] = filterFunc(this->bytes[i]);
    }
    return result;
}

PLY_NO_INLINE String StringView::join(ArrayView<const StringView> comps) const {
    MemOutStream mout;
    bool first = true;
    for (StringView comp : comps) {
        if (!first) {
            mout.write(*this);
        }
        mout.write(comp);
        first = false;
    }
    return mout.moveToString();
}

PLY_NO_INLINE HybridString StringView::withNullTerminator() const {
    if (this->includesNullTerminator()) {
        return *this;
    }
    String result = String::allocate(this->numBytes + 1);
    memcpy(result.bytes, this->bytes, this->numBytes);
    result.bytes[this->numBytes] = 0;
    return result;
}

PLY_NO_INLINE StringView StringView::withoutNullTerminator() const {
    if (!this->includesNullTerminator()) {
        return *this;
    }
    return {this->bytes, this->numBytes - 1};
}

PLY_NO_INLINE s32 compare(StringView a, StringView b) {
    u32 compareBytes = min(a.numBytes, b.numBytes);
    const u8* u0 = (const u8*) a.bytes;
    const u8* u1 = (const u8*) b.bytes;
    const u8* uEnd0 = u0 + compareBytes;
    while (u0 < uEnd0) {
        s32 diff = *u0 - *u1;
        if (diff != 0)
            return diff;
        u0++;
        u1++;
    }
    return a.numBytes - b.numBytes;
}

PLY_NO_INLINE String operator+(StringView a, StringView b) {
    String result = String::allocate(a.numBytes + b.numBytes);
    memcpy(result.bytes, a.bytes, a.numBytes);
    memcpy(result.bytes + a.numBytes, b.bytes, b.numBytes);
    return result;
}

PLY_NO_INLINE String operator*(StringView str, u32 count) {
    String result = String::allocate(str.numBytes * count);
    char* dst = result.bytes;
    for (u32 i = 0; i < count; i++) {
        memcpy(dst, str.bytes, str.numBytes);
        dst += str.numBytes;
    }
    return result;
}

} // namespace ply

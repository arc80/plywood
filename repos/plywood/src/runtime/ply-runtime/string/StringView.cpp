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

PLY_NO_INLINE String StringView::operator+(StringView other) const {
    String result = String::allocate(this->numBytes + other.numBytes);
    memcpy(result.bytes, this->bytes, this->numBytes);
    memcpy(result.bytes + this->numBytes, other.bytes, other.numBytes);
    return result;
}

PLY_NO_INLINE String StringView::operator*(u32 count) const {
    String result = String::allocate(this->numBytes * count);
    char* dst = result.bytes;
    for (u32 i = 0; i < count; i++) {
        memcpy(dst, this->bytes, this->numBytes);
        dst += this->numBytes;
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

PLY_NO_INLINE String StringView::reversedUTF8() const {
    // When reversing badly-encoded UTF-8 strings, we just reverse the bad bytes.
    // That means, when dealing with bad UTF-8 strings, two calls to reversedUTF8() *might* not
    // return the same string.
    String result = String::allocate(this->numBytes);
    StringView srcView = *this;
    MutableStringView dstView{result.bytes, result.numBytes};
    while (dstView.numBytes > 0) {
        u32 bytesToCopy = UTF8::backNumBytes(srcView);
        PLY_ASSERT(bytesToCopy > 0);
        for (u32 i = 0; i < bytesToCopy; i++) {
            dstView.bytes[i] = srcView.bytes[srcView.numBytes - (s32) bytesToCopy + i];
        }
        dstView.offsetHead(bytesToCopy);
        srcView.offsetBack(-(s32) bytesToCopy);
    }
    PLY_ASSERT(srcView.numBytes == 0);
    return result;
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

PLY_NO_INLINE s32 compare(StringView str0, StringView str1) {
    // Returns:
    // -1 if str0 < str1
    // 0 if str0 == str1
    // 1 if str0 > str1
    u32 compareBytes = min(str0.numBytes, str1.numBytes);
    const u8* u0 = (const u8*) str0.bytes;
    const u8* u1 = (const u8*) str1.bytes;
    const u8* uEnd0 = u0 + compareBytes;
    while (u0 < uEnd0) {
        s32 diff = *u0 - *u1;
        if (diff != 0)
            return diff;
        u0++;
        u1++;
    }
    return str0.numBytes - str1.numBytes;
}

PLY_NO_INLINE bool StringView::operator==(StringView other) const {
    if (numBytes != other.numBytes)
        return false;
    return memcmp(bytes, other.bytes, numBytes) == 0;
}

} // namespace ply

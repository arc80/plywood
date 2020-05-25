/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-runtime/Precomp.h>
#include <ply-runtime/string/String.h>

namespace ply {

PLY_NO_INLINE String::String(StringView other)
    : bytes{(char*) PLY_HEAP.alloc(other.numBytes)}, numBytes{other.numBytes} {
    memcpy(this->bytes, other.bytes, other.numBytes);
}

PLY_NO_INLINE String::String(HybridString&& other) : bytes{other.bytes}, numBytes{other.numBytes} {
    if (!other.isOwner) {
        this->bytes = (char*) PLY_HEAP.alloc(other.numBytes);
        memcpy(this->bytes, other.bytes, other.numBytes);
    } else {
        other.bytes = nullptr;
        other.isOwner = 0;
        other.numBytes = 0;
    }
}

PLY_NO_INLINE String String::allocate(u32 numBytes) {
    String result;
    result.bytes = (char*) PLY_HEAP.alloc(numBytes);
    result.numBytes = numBytes;
    return result;
}

PLY_NO_INLINE void String::resize(u32 numBytes) {
    this->bytes = (char*) PLY_HEAP.realloc(this->bytes, numBytes);
    this->numBytes = numBytes;
}

} // namespace ply

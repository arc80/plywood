/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-runtime.h>

namespace ply {

PLY_NO_INLINE String::String(StringView other)
    : bytes{(char*) Heap.alloc(other.numBytes)}, numBytes{other.numBytes} {
    memcpy(this->bytes, other.bytes, other.numBytes);
}

PLY_NO_INLINE String::String(HybridString&& other) : bytes{other.bytes}, numBytes{other.numBytes} {
    if (!other.isOwner) {
        this->bytes = (char*) Heap.alloc(other.numBytes);
        memcpy(this->bytes, other.bytes, other.numBytes);
    } else {
        other.bytes = nullptr;
        other.isOwner = 0;
        other.numBytes = 0;
    }
}

PLY_NO_INLINE String String::allocate(u32 numBytes) {
    String result;
    result.bytes = (char*) Heap.alloc(numBytes);
    result.numBytes = numBytes;
    return result;
}

PLY_NO_INLINE void String::resize(u32 numBytes) {
    this->bytes = (char*) Heap.realloc(this->bytes, numBytes);
    this->numBytes = numBytes;
}

PLY_NO_INLINE HybridString::HybridString(const HybridString& other)
    : bytes{other.bytes}, isOwner{other.isOwner}, numBytes{other.numBytes} {
    if (isOwner) {
        this->bytes = String{other.view()}.release();
    }
}

} // namespace ply

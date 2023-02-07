/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-runtime.h>

namespace ply {

String::String(StringView other)
    : bytes{(char*) Heap.alloc(other.num_bytes)}, num_bytes{other.num_bytes} {
    memcpy(this->bytes, other.bytes, other.num_bytes);
}

String::String(HybridString&& other) : bytes{other.bytes}, num_bytes{other.num_bytes} {
    if (!other.is_owner) {
        this->bytes = (char*) Heap.alloc(other.num_bytes);
        memcpy(this->bytes, other.bytes, other.num_bytes);
    } else {
        other.bytes = nullptr;
        other.is_owner = 0;
        other.num_bytes = 0;
    }
}

String String::allocate(u32 num_bytes) {
    String result;
    result.bytes = (char*) Heap.alloc(num_bytes);
    result.num_bytes = num_bytes;
    return result;
}

void String::resize(u32 num_bytes) {
    this->bytes = (char*) Heap.realloc(this->bytes, num_bytes);
    this->num_bytes = num_bytes;
}

HybridString::HybridString(const HybridString& other)
    : bytes{other.bytes}, is_owner{other.is_owner}, num_bytes{other.num_bytes} {
    if (is_owner) {
        this->bytes = String{other.view()}.release();
    }
}

} // namespace ply

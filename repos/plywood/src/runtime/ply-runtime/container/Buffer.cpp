/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-runtime/Precomp.h>
#include <ply-runtime/container/Buffer.h>
#include <ply-runtime/container/ChunkList.h>

namespace ply {

PLY_NO_INLINE Buffer::Buffer(ConstBufferView other)
    : bytes{(u8*) PLY_HEAP.alloc(other.numBytes)}, numBytes{other.numBytes} {
    memcpy(bytes, other.bytes, other.numBytes);
}

} // namespace ply

/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-runtime/Precomp.h>
#include <ply-runtime/container/BufferView.h>
#include <string.h>

namespace ply {

PLY_NO_INLINE bool ConstBufferView::operator==(ConstBufferView other) const {
    if (numBytes != other.numBytes)
        return false;
    return memcmp(bytes, other.bytes, numBytes) == 0;
}

} // namespace ply

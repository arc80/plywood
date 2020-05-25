/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-runtime/Precomp.h>
#include <ply-runtime/container/Boxed.h>

namespace ply {

Buffer Boxed<Buffer>::read(NativeEndianReader& rd) {
    u32 numBytes = rd.read<u32>();
    if (rd.ins->atEOF())
        return {};
    Buffer bin;
    bin.resize(numBytes);
    rd.ins->read(bin);
    return bin;
}

} // namespace ply

/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-runtime/Precomp.h>
#include <ply-runtime/container/Boxed.h>

namespace ply {

String Boxed<String>::read(NativeEndianReader& rd) {
    u32 numBytes = rd.read<u32>();
    if (rd.ins->atEOF())
        return {};
    String bin = String::allocate(numBytes);
    rd.ins->read({bin.bytes, bin.numBytes});
    return bin;
}

} // namespace ply

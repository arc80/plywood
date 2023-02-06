/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-runtime/Precomp.h>
#include <ply-runtime/container/Boxed.h>

namespace ply {

String Boxed<String>::read(NativeEndianReader& rd) {
    u32 num_bytes = rd.read<u32>();
    if (rd.in.at_eof())
        return {};
    String bin = String::allocate(num_bytes);
    rd.in.read({bin.bytes, bin.num_bytes});
    return bin;
}

} // namespace ply

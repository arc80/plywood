/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                   ┃
┃    ╱   ╱╲    Plywood Multimedia Toolkit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/          ┃
┃    └──┴┴┴┘                                 ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/

#include <ply-runtime/Precomp.h>
#include <ply-runtime/io/Pipe.h>

namespace ply {

u64 InPipe::get_file_size() {
    // This method is unsupported by the subclass. Do not call.
    PLY_ASSERT(0);
    return 0;
}

void OutPipe::flush(bool) {
    // Does nothing.
}

bool fill_buffer(MutStringView to_buf, InPipe* from_pipe) {
    while (to_buf.numBytes > 0) {
        u32 rc = from_pipe->read(to_buf);
        if (rc == 0)
            return false;
        PLY_ASSERT(rc <= to_buf.numBytes);
        to_buf.offsetHead(rc);
    }
    return true;
}

} // namespace ply

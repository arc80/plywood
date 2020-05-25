/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-runtime/Precomp.h>
#include <ply-runtime/io/Pipe.h>

namespace ply {

PLY_NO_INLINE bool InPipe::read(BufferView buf) {
    while (buf.numBytes > 0) {
        u32 rc = this->readSome(buf);
        if (rc == 0)
            return false;
        PLY_ASSERT(rc <= buf.numBytes);
        buf.offsetHead(rc);
    }
    return true;
}

PLY_NO_INLINE u64 InPipe::getFileSize_Unsupported(const InPipe*) {
    PLY_ASSERT(0); // Shouldn't call getFileSize() on this pipe
    return 0;
}

PLY_NO_INLINE void OutPipe::flush_Empty(OutPipe*) {
}

PLY_NO_INLINE u64 OutPipe::seek_Empty(OutPipe*, s64, SeekDir) {
    return 0;
}

} // namespace ply

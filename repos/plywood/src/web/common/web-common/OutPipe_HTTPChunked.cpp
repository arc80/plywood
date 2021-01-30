/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <web-common/Core.h>
#include <web-common/OutPipe_HTTPChunked.h>

namespace ply {
namespace web {

PLY_NO_INLINE void OutPipe_HTTPChunked_destroy(OutPipe* outPipe_) {
    OutPipe_HTTPChunked* outPipe = static_cast<OutPipe_HTTPChunked*>(outPipe_);
    // End of chunk stream
    *outPipe->outs->strWriter() << "0\r\n\r\n";
    outPipe->outs->flushMem();
}

PLY_NO_INLINE bool OutPipe_HTTPChunked_write(OutPipe* outPipe_, StringView srcBuf) {
    OutPipe_HTTPChunked* outPipe = static_cast<OutPipe_HTTPChunked*>(outPipe_);
    if (outPipe->chunkMode) {
        PLY_ASSERT(srcBuf.numBytes > 0);
        outPipe->outs->strWriter()->format("{}\r\n", fmt::Hex{srcBuf.numBytes, true});
    }
    outPipe->outs->write(srcBuf);
    if (outPipe->chunkMode) {
        *outPipe->outs->strWriter() << "\r\n";
    }
    return !outPipe->outs->atEOF();
}

PLY_NO_INLINE bool OutPipe_HTTPChunked_flush(OutPipe* outPipe_, bool toDevice) {
    OutPipe_HTTPChunked* outPipe = static_cast<OutPipe_HTTPChunked*>(outPipe_);
    return outPipe->outs->flush(toDevice);
}

OutPipe::Funcs OutPipe_HTTPChunked::Funcs_ = {
    OutPipe_HTTPChunked_destroy,
    OutPipe_HTTPChunked_write,
    OutPipe_HTTPChunked_flush,
    OutPipe::seek_Empty,
};

} // namespace web
} // namespace ply

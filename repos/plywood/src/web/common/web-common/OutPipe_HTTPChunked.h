/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <web-common/Core.h>

namespace ply {
namespace web {

//-----------------------------------------------------------------------
// OutPipe_HTTPChunked
//-----------------------------------------------------------------------
struct OutPipe_HTTPChunked : OutPipe {
    static Funcs Funcs_;
    OptionallyOwned<OutStream> outs;
    bool chunkMode = false;

    PLY_INLINE OutPipe_HTTPChunked(OptionallyOwned<OutStream>&& outs)
        : OutPipe{&Funcs_}, outs{std::move(outs)} {
    }
    PLY_INLINE void setChunkMode(bool chunkMode) {
        this->chunkMode = chunkMode;
    }
};

} // namespace web
} // namespace ply

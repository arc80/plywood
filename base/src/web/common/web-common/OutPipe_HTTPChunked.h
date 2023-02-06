/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
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
    bool chunk_mode = false;

    PLY_INLINE OutPipe_HTTPChunked(OptionallyOwned<OutStream>&& outs)
        : OutPipe{&Funcs_}, outs{std::move(outs)} {
    }
    PLY_INLINE void set_chunk_mode(bool chunk_mode) {
        this->chunk_mode = chunk_mode;
    }
};

} // namespace web
} // namespace ply

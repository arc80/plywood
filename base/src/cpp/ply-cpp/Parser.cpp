/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-cpp/Core.h>
#include <ply-cpp/Parser.h>

namespace ply {
namespace cpp {

PLY_NO_INLINE void Parser::error(bool begin_muting, ParseError&& err) {
    raw_error_count++;
    if (this->restore_point_enabled)
        return;

    // Only invoke error handler if not muted:
    if (!this->mute_errors) {
        this->pp->error_handler(new ParseError{std::move(err)});
    }

    if (begin_muting) {
        this->mute_errors = true;
    }
}

} // namespace cpp
} // namespace ply

#include "codegen/Parser.inl" //%%

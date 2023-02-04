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

PLY_NO_INLINE void Parser::error(bool beginMuting, ParseError&& err) {
    rawErrorCount++;
    if (this->restorePointEnabled)
        return;

    // Only invoke error handler if not muted:
    if (!this->muteErrors) {
        this->pp->errorHandler(new ParseError{std::move(err)});
    }

    if (beginMuting) {
        this->muteErrors = true;
    }
}

} // namespace cpp
} // namespace ply

#include "codegen/Parser.inl" //%%

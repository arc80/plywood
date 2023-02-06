/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once
#include <ply-cpp/Core.h>
#include <ply-cpp/LinearLocation.h>
#include <ply-cpp/PPVisitedFiles.h>

namespace ply {
namespace cpp {

struct BaseError {
    virtual ~BaseError() {
    }
    virtual void write_message(OutStream& out,
                               const PPVisitedFiles* visited_files) const = 0;
};

} // namespace cpp
} // namespace ply

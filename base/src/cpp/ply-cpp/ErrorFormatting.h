/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once
#include <ply-cpp/Core.h>
#include <ply-cpp/PPVisitedFiles.h>

namespace ply {
namespace cpp {

struct ExpandedFileLocation {
    const PPVisitedFiles::SourceFile* src_file;
    FileLocation file_loc;

    String to_string() const;
};

ExpandedFileLocation expand_file_location(const PPVisitedFiles* visited_files,
                                          LinearLocation linear_loc);

} // namespace cpp
} // namespace ply

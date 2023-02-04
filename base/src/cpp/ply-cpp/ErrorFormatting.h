﻿/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
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
    const PPVisitedFiles::SourceFile* srcFile;
    FileLocation fileLoc;

    String toString() const;
};

ExpandedFileLocation expandFileLocation(const PPVisitedFiles* visitedFiles,
                                        LinearLocation linearLoc);

} // namespace cpp
} // namespace ply

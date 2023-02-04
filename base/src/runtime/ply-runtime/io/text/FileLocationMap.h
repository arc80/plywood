﻿/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once
#include <ply-runtime.h>

namespace ply {

struct FileLocation {
    u32 lineNumber;
    u32 numBytesIntoLine;
    u32 columnNumber : 28;
    u32 numBytesIntoColumn : 4; // Will be non-zero if the FileLocation lands in the middle of a
                                // multibyte character (may happen when indexing the file)

    FileLocation(u32 lineNumber, u32 numBytesIntoLine, u32 columnNumber, u32 numBytesIntoColumn)
        : lineNumber{lineNumber}, numBytesIntoLine{numBytesIntoLine}, columnNumber{columnNumber},
          numBytesIntoColumn{numBytesIntoColumn} {
    }
};

struct FileLocationMap {
    String path;
    Array<FileLocation> table;
    StringView view;

    static FileLocationMap fromView(StringView path, StringView view);
    FileLocation getFileLocation(u32 offset) const;
    String formatFileLocation(u32 offset, bool withPath = true) const;
};

} // namespace ply

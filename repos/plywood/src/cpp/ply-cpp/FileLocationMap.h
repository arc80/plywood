/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-cpp/Core.h>

namespace ply {
namespace cpp {

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
    Array<FileLocation> table;
    StringView view;

    static FileLocationMap fromView(StringView view);
    FileLocation getFileLocation(u32 offset) const;
};

} // namespace cpp
} // namespace ply

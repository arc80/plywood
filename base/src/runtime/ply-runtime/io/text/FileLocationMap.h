/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once
#include <ply-runtime.h>

namespace ply {

struct FileLocation {
    u32 line_number;
    u32 num_bytes_into_line;
    u32 column_number : 28;
    u32 num_bytes_into_column : 4; // Will be non-zero if the FileLocation lands in the
                                   // middle of a multibyte character (may happen when
                                   // indexing the file)

    FileLocation(u32 line_number, u32 num_bytes_into_line, u32 column_number,
                 u32 num_bytes_into_column)
        : line_number{line_number}, num_bytes_into_line{num_bytes_into_line},
          column_number{column_number}, num_bytes_into_column{num_bytes_into_column} {
    }
};

struct FileLocationMap {
    String path;
    Array<FileLocation> table;
    StringView view;

    static FileLocationMap from_view(StringView path, StringView view);
    FileLocation get_file_location(u32 offset) const;
    String format_file_location(u32 offset, bool with_path = true) const;
};

} // namespace ply

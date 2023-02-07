/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-runtime/Precomp.h>
#include <ply-runtime/io/text/FileLocationMap.h>

namespace ply {

FileLocationMap FileLocationMap::from_view(StringView path, StringView src) {
    ViewInStream src_in{src};

    FileLocationMap result;
    result.path = path;
    result.view = src;
    u32 line_number = 1;
    u32 column_number = 1;
    u32 line_start_ofs = 0;

    u32 ofs = 0;
    u32 next_chunk_ofs = 256;
    result.table.append({1, 0, 1, 0});
    for (;;) {
        s32 codepoint = Unicode{UTF8}.decode_point(src_in);
        if (codepoint < 0)
            break;
        u32 next_ofs = check_cast<u32>(src_in.cur_byte - src_in.start_byte);
        if (next_ofs > next_chunk_ofs) {
            result.table.append({line_number, next_chunk_ofs - line_start_ofs,
                                 column_number, ofs - 256});
            next_chunk_ofs += 256;
        }
        ofs = next_ofs;

        if (codepoint == '\n') {
            line_number++;
            column_number = 1;
            line_start_ofs = ofs;
        } else if (codepoint == '\t') {
            u32 tab_size = 4;
            column_number += tab_size - (column_number % tab_size);
        } else if (codepoint == '\r') {
        } else {
            column_number++;
        }
    }
    return result;
}

FileLocation FileLocationMap::get_file_location(u32 offset) const {
    PLY_ASSERT(offset <= this->view.num_bytes);
    const FileLocation& file_loc = this->table[offset >> 8];
    u32 chunk_ofs = offset & ~0xff;
    const char* line_start =
        this->view.bytes + (chunk_ofs - file_loc.num_bytes_into_line);
    StringView src = this->view;
    src.offset_head(chunk_ofs - file_loc.num_bytes_into_column);
    ViewInStream src_in{src};
    const char* target = this->view.bytes + offset;
    u32 line_number = file_loc.line_number;
    u32 column_number = file_loc.column_number;

    for (;;) {
        if (src_in.cur_byte >= target) {
            u32 nb = check_cast<u32>(target - src.bytes);
            return {line_number, check_cast<u32>(target - line_start), column_number,
                    nb};
        }

        // FIXME: Unify with similar code in previous function
        u32 codepoint = Unicode{UTF8}.decode_point(src_in);

        if (codepoint == '\n') {
            line_number++;
            column_number = 1;
            line_start = src.bytes;
        } else if (codepoint == '\t') {
            u32 tab_size = 4; // FIXME: Make configurable somewhere
            column_number += tab_size - (column_number % tab_size);
        } else if (codepoint == '\r') {
        } else {
            column_number++;
        }
    }
}

String FileLocationMap::format_file_location(u32 offset, bool with_path) const {
    FileLocation file_loc = this->get_file_location(offset);
    return String::format("{}({}, {})", (with_path ? this->path : ""),
                          file_loc.line_number, file_loc.column_number);
}

} // namespace ply

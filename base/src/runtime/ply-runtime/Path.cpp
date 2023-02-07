/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-runtime/Precomp.h>
#include <ply-runtime/string/WString.h>

namespace ply {

#if PLY_TARGET_WIN32
Path_t Path{true};
#elif PLY_TARGET_POSIX
Path_t Path{false};
#else
#error "Unsupported target!"
#endif

Path_t WindowsPath{true};
Path_t PosixPath{false};

Tuple<StringView, StringView> Path_t::split(StringView path) const {
    s32 last_sep_index = path.rfind_byte([&](char c) { return this->is_sep_byte(c); });
    if (last_sep_index >= 0) {
        s32 prefix_len = path.rfind_byte([&](char c) { return !this->is_sep_byte(c); },
                                         last_sep_index) +
                         1;
        if (path.left(prefix_len) == this->get_drive_letter(path)) {
            prefix_len++; // If prefix is the root, include a separator character
        }
        return {path.left(prefix_len), path.sub_str(last_sep_index + 1)};
    } else {
        return {String{}, path};
    }
}

Array<StringView> Path_t::split_full(StringView path) const {
    Array<StringView> result;
    if (this->has_drive_letter(path)) {
        if (this->is_absolute(path)) {
            // Root with drive letter
            result.append(path.left(3));
            path.offset_head(3);
            while (path.num_bytes > 0 && this->is_sep_byte(path[0])) {
                path.offset_head(1);
            }
        } else {
            // Drive letter only
            result.append(path.left(2));
            path.offset_head(2);
        }
    } else if (path.num_bytes > 0 && this->is_sep_byte(path[0])) {
        // Starts with path separator
        result.append(path.left(1));
        path.offset_head(1);
        while (path.num_bytes > 0 && this->is_sep_byte(path[0])) {
            path.offset_head(1);
        }
    }
    if (path.num_bytes > 0) {
        for (;;) {
            PLY_ASSERT(path.num_bytes > 0);
            PLY_ASSERT(!this->is_sep_byte(path[0]));
            s32 sep_pos = path.find_byte([&](char c) { return this->is_sep_byte(c); });
            if (sep_pos < 0) {
                result.append(path);
                break;
            }
            result.append(path.left(sep_pos));
            path.offset_head(sep_pos);
            s32 non_sep_pos =
                path.find_byte([&](char c) { return !this->is_sep_byte(c); });
            if (non_sep_pos < 0) {
                // Empty final component
                result.append({});
                break;
            }
            path.offset_head(non_sep_pos);
        }
    }
    return result;
}

Tuple<StringView, StringView> Path_t::split_ext(StringView path) const {
    StringView last_comp = path;
    s32 slash_pos = last_comp.rfind_byte([&](char c) { return this->is_sep_byte(c); });
    if (slash_pos >= 0) {
        last_comp = last_comp.sub_str(slash_pos + 1);
    }
    s32 dot_pos = last_comp.rfind_byte([](u32 c) { return c == '.'; });
    if (dot_pos < 0 || dot_pos == 0) {
        dot_pos = last_comp.num_bytes;
    }
    return {path.shortened_by(last_comp.num_bytes - dot_pos),
            last_comp.sub_str(dot_pos)};
}

struct PathCompIterator {
    char first_comp[3] = {0};

    void iterate_over(const Path_t* path_fmt, ArrayView<const StringView> components,
                      const Func<void(StringView)>& callback) {
        s32 absolute_index = -1;
        s32 drive_letter_index = -1;
        for (s32 i = components.num_items - 1; i >= 0; i--) {
            if (absolute_index < 0 && path_fmt->is_absolute(components[i])) {
                absolute_index = i;
            }
            if (path_fmt->has_drive_letter(components[i])) {
                drive_letter_index = i;
                break;
            }
        }

        // Special first component if there's a drive letter and/or absolute component:
        if (drive_letter_index >= 0) {
            first_comp[0] = components[drive_letter_index][0];
            first_comp[1] = ':';
            if (absolute_index >= 0) {
                first_comp[2] = path_fmt->sep_byte();
                callback(StringView{first_comp, 3});
            } else {
                callback(StringView{first_comp, 2});
            }
        }

        // Choose component to start iterating from:
        u32 i = drive_letter_index >= 0 ? drive_letter_index : 0;
        if (absolute_index >= 0) {
            PLY_ASSERT((u32) absolute_index >= i);
            i = absolute_index;
            if (drive_letter_index < 0) {
                char sep_byte = path_fmt->sep_byte();
                callback(StringView{&sep_byte, 1});
            }
        }

        // Iterate over components. Remember, we've already sent the drive letter and/or
        // initial slash as its own component (if any).
        for (; i < components.num_items; i++) {
            StringView comp = components[i];
            if ((s32) i == drive_letter_index) {
                comp = comp.sub_str(2);
            }

            s32 non_sep = comp.find_byte(
                [path_fmt](char c) { return !path_fmt->is_sep_byte(c); });
            while (non_sep >= 0) {
                s32 sep = comp.find_byte(
                    [path_fmt](char c) { return path_fmt->is_sep_byte(c); },
                    non_sep + 1);
                if (sep < 0) {
                    callback(comp.sub_str(non_sep));
                    break;
                } else {
                    callback(comp.sub_str(non_sep, sep - non_sep));
                    non_sep = comp.find_byte(
                        [path_fmt](char c) { return !path_fmt->is_sep_byte(c); },
                        sep + 1);
                }
            }
        }
    }

    // Note: Keep the PathCompIterator alive while using the return value
    Array<StringView> get_normalized_comps(const Path_t* path_fmt,
                                           ArrayView<const StringView> components) {
        Array<StringView> norm_comps;
        u32 up_count = 0;
        this->iterate_over(path_fmt, components, [&](StringView comp) { //
            if (comp == "..") {
                if (norm_comps.num_items() > up_count) {
                    norm_comps.pop();
                } else {
                    PLY_ASSERT(norm_comps.num_items() == up_count);
                    norm_comps.append("..");
                }
            } else if (comp != "." && !comp.is_empty()) {
                norm_comps.append(comp);
            }
        });
        return norm_comps;
    }
};

String Path_t::join_array(ArrayView<const StringView> components) const {
    PathCompIterator comp_iter;
    Array<StringView> norm_comps = comp_iter.get_normalized_comps(this, components);
    if (norm_comps.is_empty()) {
        if (components.num_items > 0 && components.back().is_empty()) {
            return StringView{"."} + this->sep_byte();
        } else {
            return ".";
        }
    } else {
        MemOutStream out;
        bool need_sep = false;
        for (StringView comp : norm_comps) {
            if (need_sep) {
                out << this->sep_byte();
            } else {
                if (comp.num_bytes > 0) {
                    need_sep = !this->is_sep_byte(comp[comp.num_bytes - 1]);
                }
            }
            out << comp;
        }
        if ((components.back().is_empty() ||
             this->is_sep_byte(components.back().back())) &&
            need_sep) {
            out << this->sep_byte();
        }
        return out.move_to_string();
    }
}

String Path_t::make_relative(StringView ancestor, StringView descendant) const {
    // This function requires either both absolute paths or both relative paths:
    PLY_ASSERT(this->is_absolute(ancestor) == this->is_absolute(descendant));

    // FIXME: Implement fastpath when descendant starts with ancestor and there are no
    // ".", ".." components.

    PathCompIterator ancestor_comp_iter;
    Array<StringView> ancestor_comps =
        ancestor_comp_iter.get_normalized_comps(this, {ancestor});
    PathCompIterator descendant_comp_iter;
    Array<StringView> descendant_comps =
        descendant_comp_iter.get_normalized_comps(this, {descendant});

    // Determine number of matching components
    u32 mc = 0;
    while (mc < ancestor_comps.num_items() && mc < descendant_comps.num_items()) {
        if (ancestor_comps[mc] != descendant_comps[mc])
            break;
        mc++;
    }

    // Determine number of ".." to output (will be 0 if drive letters mismatch)
    u32 up_folders = 0;
    if (!this->is_absolute(ancestor) || mc > 0) {
        up_folders = ancestor_comps.num_items() - mc;
    }

    // Form relative path (or absolute path if drive letters mismatch)
    MemOutStream out;
    bool need_sep = false;
    for (u32 i = 0; i < up_folders; i++) {
        if (need_sep) {
            out << this->sep_byte();
        }
        out << "..";
        need_sep = true;
    }
    for (u32 i = mc; i < descendant_comps.num_items(); i++) {
        if (need_sep) {
            out << this->sep_byte();
        }
        out << descendant_comps[i];
        need_sep = !this->is_sep_byte(descendant_comps[i].back());
    }

    // .
    if (out.get_seek_pos() == 0) {
        out << ".";
        need_sep = true;
    }

    // Trailing slash
    if (descendant.num_bytes > 0 && this->is_sep_byte(descendant.back()) && need_sep) {
        out << this->sep_byte();
    }

    return out.move_to_string();
}

HybridString Path_t::from(const Path_t& src_format, StringView src_path) const {
    if (this->is_windows == src_format.is_windows)
        return src_path;
    String result = src_path;
    char* bytes = result.bytes;
    for (u32 i = 0; i < result.num_bytes; i++) {
        if (src_format.is_sep_byte(bytes[i])) {
            bytes[i] = this->sep_byte();
        }
    }
    return result;
}

WString win32_path_arg(StringView path, bool allow_extended) {
    ViewInStream path_in{path};
    MemOutStream out;
    if (allow_extended && WindowsPath.is_absolute(path)) {
        out << ArrayView<const char16_t>{u"\\\\?\\", 4}.string_view();
    }
    while (true) {
        s32 codepoint = Unicode{UTF8}.decode_point(path_in);
        if (codepoint < 0)
            break;
        if (codepoint == '/') {
            codepoint = '\\'; // Fix slashes.
        }
        Unicode{UTF16_LE}.encode_point(out, codepoint);
    }
    out.raw_write<u16>(0); // Null terminator.
    return WString::move_from_string(out.move_to_string());
}

} // namespace ply

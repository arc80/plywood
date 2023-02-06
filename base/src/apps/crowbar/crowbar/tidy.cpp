/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include "core.h"
#include <ply-runtime.h>
#include <ply-build-repo/Workspace.h>
#include <ply-cpp/Preprocessor.h>
#include <ply-cpp/ErrorFormatting.h>
#include <ply-runtime/tokenizer.h>

void update_file_header(StringView src_path) {
    String src = FileSystem.load_text_autodetect(src_path);
    if (FileSystem.last_result() != FSResult::OK)
        return;

    ViewInStream in{src};
    const char* end_of_header = in.cur_byte;
    while (StringView line = in.read_view<fmt::Line>()) {
        StringView trimmed = line.trim();
        if (trimmed.is_empty() || trimmed.starts_with('#'))
            break;
        end_of_header = in.cur_byte;
    }

    StringView desired_header =
        u8R"(/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
)";
    String new_src =
        desired_header + StringView::from_range(end_of_header, in.end_byte);
    FileSystem.make_dirs_and_save_text_if_different(src_path, new_src,
                                                    Workspace.get_source_text_format());
}

bool is_camel_case(StringView identifier) {
    if (identifier.num_bytes < 2)
        return false;
    char c = identifier[0];
    if (c >= 'a' && c <= 'z') {
        for (u32 i = 1; i < identifier.num_bytes; i++) {
            c = identifier[i];
            if (c < 0)
                return false; // Multibyte UTF-8
            if (c == '_' && (i + 1) < identifier.num_bytes)
                return false; // Snake case
            if (c >= 'A' && c <= 'Z')
                return true;
        }
    }
    return false;
}

String camel_to_snake_case(StringView identifier) {
    MemOutStream out;
    bool needs_underscore = false;
    for (char c : identifier) {
        if (c >= 'A' && c <= 'Z') {
            if (needs_underscore) {
                out << '_';
                needs_underscore = false;
            }
            out << char(c + 'a' - 'A');
        } else {
            needs_underscore = (c != '_');
            out << c;
        }
    }
    return out.move_to_string();
}

void build_camel_to_snake_map(Map<String, String>& map, StringView src_path) {
    String src = FileSystem.load_text_autodetect(src_path);
    if (FileSystem.last_result() != FSResult::OK)
        return;

    Tokenizer tkr{ViewInStream{src}};
    const char* tok_start = tkr.in.cur_byte;
    while (TokenKind kind = tkr.read_token()) {
        if (kind == TK_Identifier) {
            StringView identifier = StringView::from_range(tok_start, tkr.in.cur_byte);
            if (is_camel_case(identifier) && !map.find(identifier)) {
                String snake_case = camel_to_snake_case(identifier);
                map.assign(identifier, snake_case);
            }
        }
        tok_start = tkr.in.cur_byte;
    }
}

void replace_identifiers_in_file(const Map<String, String>& map, StringView src_path) {
    String src = FileSystem.load_text_autodetect(src_path);
    if (FileSystem.last_result() != FSResult::OK)
        return;

    MemOutStream out;
    Tokenizer tkr{ViewInStream{src}};
    const char* tok_start = tkr.in.cur_byte;
    while (TokenKind kind = tkr.read_token()) {
        StringView src_text = StringView::from_range(tok_start, tkr.in.cur_byte);
        tok_start = tkr.in.cur_byte;
        if (kind == TK_Identifier) {
            if (const String* replacement = map.find(src_text)) {
                out << *replacement;
                continue;
            }
        }
        out << src_text;
    }

    FileSystem.make_dirs_and_save_text_if_different(src_path, out.move_to_string(),
                                                    Workspace.get_source_text_format());
}

void save_identifier_map(Map<String, String>& map, StringView path) {
    sort(map.items, [](const auto& a, const auto& b) { return a.key < b.key; });
    MemOutStream out;
    for (const auto item : map.items) {
        out.format("{},{}\n", item.key, item.value);
    }
    FileSystem.make_dirs_and_save_text_if_different(path, out.move_to_string());
}

Map<String, String> load_identifier_map(StringView path) {
    Map<String, String> map;
    String src = FileSystem.load_text_autodetect(path);
    ViewInStream in{src};
    while (StringView line = in.read_view<fmt::Line>()) {
        Array<StringView> parts = line.trim().split_byte(',');
        map.assign(parts[0], parts[1]);
    }
    return map;
}

void tidy_source() {
    String root = Path.join(Workspace.path, "base/src");

    // String identifier_map_path = Path.join(Workspace.path, "identifier_map.txt");
    // Map<String, String> identifier_map;
    // identifier_map = load_identifier_map(identifier_map_path);

    for (WalkTriple& triple : FileSystem.walk(root)) {
        for (const FileInfo& file : triple.files) {
            if (file.name.ends_with(".cpp") || file.name.ends_with(".inl") ||
                file.name.ends_with(".h")) {
                String path = Path.join(triple.dir_path, file.name);

                // Run clang-format
                // if (Owned<Process> sub = Process::exec(
                //        "C:/Program Files/Microsoft Visual "
                //        "Studio/2022/Community/VC/Tools/Llvm/x64/bin/clang-format",
                //        {"-i", file.name}, triple.dir_path,
                //        Process::Output::inherit())) {
                //    sub->join();
                // }

                // Update file header
                // update_file_header(path);

                // Convert camelCase to snake_case
                // build_camel_to_snake_map(identifier_map, path);
                // replace_identifiers_in_file(identifier_map, path);
            }
        }
    }

    // save_identifier_map(identifier_map, identifier_map_path);
}

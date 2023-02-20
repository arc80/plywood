/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-build-repo/Workspace.h>
#include <ply-runtime/tokenizer.h>

namespace ply {
namespace build {

struct LabelPreprocessor {
    // FIXME: Replace with Set<>
    Map<String, bool> labels;

    void add_from_file(StringView path) {
        String src = FileSystem.load_text_autodetect(path);
        if (FileSystem.last_result() != FSResult::OK)
            return;

        Tokenizer tkr{ViewInStream{src}};
        const char* tok_start = tkr.in.cur_byte;
        while (TokenKind kind = tkr.read_token()) {
            if (kind == TK_Identifier) {
                StringView identifier =
                    StringView::from_range(tok_start, tkr.in.cur_byte);
                if (identifier.starts_with("LABEL_") && (identifier.num_bytes > 6)) {
                    this->labels.assign(identifier.sub_str(6), true);
                }
            }
            tok_start = tkr.in.cur_byte;
        }
    }
};

FnResult preprocess_labels(const FnParams& params) {
    // Collect all identifiers that start with "LABEL_".
    LabelPreprocessor lp;
    String root = Path.join(Workspace.path, "base/src");
    for (WalkTriple& triple : FileSystem.walk(root)) {
        for (const FileInfo& file : triple.files) {
            if (file.name.ends_with(".cpp") || file.name.ends_with(".h")) {
                String path = Path.join(triple.dir_path, file.name);
                lp.add_from_file(path);
            }
        }
    }

    // Sort list.
    sort(lp.labels.items, [](const auto a, const auto b) { return a.key < b.key; });

    // Write predefined_labels.cpp and .h.
    MemOutStream source;
    MemOutStream header;
    source << R"#(#include <ply-runtime.h>
#include <ply-runtime/predefined_labels.h>

namespace ply {

#define PLY_INIT_PREDEFINED_LABEL(text) \
    { \
        u32 idx = g_labelStorage.insert(#text).idx; \
        PLY_ASSERT(idx == LABEL_##text.idx); \
        PLY_UNUSED(idx); \
    }

void init_predefined_labels() {
)#";
    u32 idx = 2;
    for (const auto& item : lp.labels.items) {
        header.format("constexpr Label LABEL_{}{{{}}};\n", item.key, idx);
        source.format("    PLY_INIT_PREDEFINED_LABEL({});\n", item.key);
        idx += align_power_of2(item.key.num_bytes + 2, 2);
    }
    source << R"#(}

#undef PLY_INIT_PREDEFINED_LABEL

} // namespace ply
)#";
    String path = Path.join(Workspace.path, "data/build",
                            Workspace.current_build_folder, "codegen/runtime");
    FileSystem.make_dirs_and_save_text_if_different(
        Path.join(path, "ply-runtime/predefined_labels.h"), header.move_to_string());
    FileSystem.make_dirs_and_save_text_if_different(
        Path.join(path, "ply-runtime/predefined_labels.cpp"), source.move_to_string());

    return Fn_OK;
}

} // namespace build
} // namespace ply

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
                StringView identifier = StringView::from_range(tok_start, tkr.in.cur_byte);
                if (identifier.starts_with("LABEL_")) {
                    this->labels.assign(identifier, true);
                }
            }
            tok_start = tkr.in.cur_byte;
        }
    }
};

FnResult preprocess_labels(const FnParams& params) {
    LabelPreprocessor lp;
    String root = Path.join(Workspace.path, "base/src");
    for (WalkTriple& triple : FileSystem.walk(root)) {
        for (const FileInfo& file : triple.files) {
            if (file.name.ends_with("TypeInfo.h"))
                continue;
            if (file.name.ends_with(".cpp") || file.name.ends_with(".h")) {
                String path = Path.join(triple.dir_path, file.name);
                lp.add_from_file(path);
            }
        }
    }

    return Fn_OK;
}

} // namespace build
} // namespace ply

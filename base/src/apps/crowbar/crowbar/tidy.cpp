/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include "core.h"
#include <ply-build-repo/Workspace.h>
#include <ply-cpp/Preprocessor.h>
#include <ply-cpp/ErrorFormatting.h>

void update_file_header(StringView srcPath) {
    String src = FileSystem.loadTextAutodetect(srcPath);
    if (FileSystem.lastResult() != FSResult::OK)
        return;

    ViewInStream in{src};
    const char* end_of_header = in.cur_byte;
    while (StringView line = in.readView<fmt::Line>()) {
        StringView trimmed = line.trim();
        if (trimmed.isEmpty() || trimmed.startsWith('#'))
            break;
        end_of_header = in.cur_byte;
    }

    StringView desiredHeader =
        u8R"(/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
)";
    String new_src = desiredHeader + StringView::fromRange(end_of_header, in.end_byte);
    FileSystem.makeDirsAndSaveTextIfDifferent(srcPath, new_src, Workspace.getSourceTextFormat());
}

void tidy_source() {
    String root = Path.join(Workspace.path, "base/src");
    update_file_header(Path.join(Workspace.path,
                                 "base/src/runtime/ply-runtime/container/Hash128.cpp"));

    for (WalkTriple& triple : FileSystem.walk(root)) {
        for (const FileInfo& file : triple.files) {
            if (file.name.endsWith(".cpp") || file.name.endsWith(".h")) {
                // Run clang-format
                if (Owned<Process> sub =
                        Process::exec("clang-format", {"-i", file.name}, triple.dirPath,
                                      Process::Output::inherit())) {
                    sub->join();
                }

                // Check file header
                update_file_header(Path.join(triple.dirPath, file.name));
            }
        }
    }
}

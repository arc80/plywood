/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-reflect/Base.h>
#include <ply-runtime/Base.h>
#include <ply-cpp/ParseAPI.h>
#include <ply-cpp/PPVisitedFiles.h>
#include <ply-runtime/io/text/TextFormat.h>
#include <pylon-reflect/Export.h>
#include <pylon/Write.h>

using namespace ply;

struct ParserTestSupervisor : cpp::ParseSupervisor {
    Array<Owned<cpp::BaseError>> errors;

    virtual bool handle_error(Owned<cpp::BaseError>&& err) override {
        this->errors.append(std::move(err));
        return true;
    }
};

void run_test(StringView test_path) {
    String test_file_contents = FileSystem.load_text_autodetect(test_path).first;

    // Find the line with five dashes
    ViewInStream vins{test_file_contents};
    const char* dashed_line = nullptr;
    for (;;) {
        StringView line = vins.read_view<fmt::Line>();
        if (line.is_empty())
            break;
        if (line.starts_with("-----")) {
            dashed_line = line.bytes;
            break;
        }
    }
    if (!dashed_line) {
        // No dashed line. Consider the whole file source code:
        dashed_line = (const char*) vins.cur_byte;
    }

    StringView source_code =
        StringView::from_range(test_file_contents.bytes, dashed_line);
    // Note: We could maybe avoid this copy by changing the Preprocessor so that it
    // doesn't always take memory ownership of the input file contents.
    cpp::PPVisitedFiles visited_files;
    ParserTestSupervisor visor;
    cpp::grammar::TranslationUnit parse_result =
        cpp::parse(source_code, &visited_files, {}, {}, &visor);

    MemOutStream mout;
    mout << source_code;
    if (!source_code.ends_with("\n")) {
        mout << "\n";
    }
    mout << "-----\n";
    for (const cpp::BaseError* err : visor.errors) {
        err->write_message(&mout, &visited_files);
    }
    mout.flush_mem();
    FileSystem.make_dirs_and_save_text_if_different(test_path, mout.move_to_string(),
                                                    TextFormat::platform_preference());
}

void run_test_suite() {
    String tests_folder =
        Path.join(Workspace.path, "repos/plywood/src/apps/ParserTest/tests");
    OutStream outs = StdOut::text();
    for (const DirectoryEntry& entry : FileSystem.list_dir(tests_folder)) {
        if (!entry.is_dir && entry.name.ends_with(".txt")) {
            outs << entry.name << '\n';
            outs.flush_mem();
            run_test(Path.join(tests_folder, entry.name));
        }
    }
}

int main() {
    run_test_suite();
}

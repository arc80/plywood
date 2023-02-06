/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-runtime/Base.h>

using namespace ply;

Tuple<bool, TextFormat> extract_format_from_name(StringView name) {
    TextFormat tf;

    Array<StringView> name_comps = name.split_byte('.');
    if (name_comps.num_items() != 4)
        return {false, TextFormat{}};

    if (name_comps[1] == "utf8") {
        tf.encoding = TextFormat::Encoding::UTF8;
    } else if (name_comps[1] == "utf16le") {
        tf.encoding = TextFormat::Encoding::UTF16_le;
    } else if (name_comps[1] == "utf16be") {
        tf.encoding = TextFormat::Encoding::UTF16_be;
    } else if (name_comps[1] == "win1252") {
        tf.encoding = TextFormat::Encoding::Bytes;
    } else {
        return {false, TextFormat{}};
    }

    if (name_comps[2] == "lf") {
        tf.new_line = TextFormat::NewLine::LF;
    } else if (name_comps[2] == "crlf") {
        tf.new_line = TextFormat::NewLine::CRLF;
    } else {
        return {false, TextFormat{}};
    }

    if (name_comps[3] == "bom") {
        tf.bom = true;
    } else if (name_comps[3] == "nobom") {
        tf.bom = false;
    } else {
        return {false, TextFormat{}};
    }

    return {true, tf};
}

bool run_test_suite() {
    String tests_folder =
        Path.join(Workspace.path, "repos/plywood/src/apps/AutodetectTest/tests");
    OutStream outs = StdOut::text();
    u32 succeeded = 0;
    u32 failed = 0;
    for (const DirectoryEntry& entry : FileSystem.list_dir(tests_folder)) {
        // if (entry.name == "japanese.utf8.lf.bom.txt")
        //    PLY_DEBUG_BREAK();
        if (!entry.is_dir && entry.name.ends_with(".txt")) {
            outs << entry.name << "... ";
            Tuple<bool, TextFormat> expected =
                extract_format_from_name(entry.name.shortened_by(4));
            if (!expected.first) {
                outs << "***can't parse filename***\n";
                failed++;
                outs.flush();
                continue;
            }
            auto contents = FileSystem::native()->load_text_autodetect(
                Path.join(tests_folder, entry.name));
            if (!(contents.second == expected.second)) {
                outs << "***format detection failed***\n";
                failed++;
                outs.flush();
                continue;
            }

            auto compare_to = FileSystem::native()->load_text_autodetect(Path.join(
                tests_folder, entry.name.split_byte('.')[0] + ".utf8.crlf.bom.txt"));
            if (contents.first != compare_to.first) {
                outs << "***bad contents***\n";
                failed++;
                outs.flush();
                continue;
            }

            outs << "OK\n";
            outs.flush();
            succeeded++;
        }
    }
    outs << "----\n";
    outs.format("{} succeeded, {} failed\n", succeeded, failed);
    return failed == 0;
}

int main() {
    return run_test_suite() ? 0 : 1;
}

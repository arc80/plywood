/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-runtime/Base.h>

using namespace ply;

Tuple<bool, TextFormat> extractFormatFromName(StringView name) {
    TextFormat tf;

    Array<StringView> nameComps = name.splitByte('.');
    if (nameComps.numItems() != 4)
        return {false, TextFormat{}};

    if (nameComps[1] == "utf8") {
        tf.encoding = TextFormat::Encoding::UTF8;
    } else if (nameComps[1] == "utf16le") {
        tf.encoding = TextFormat::Encoding::UTF16_le;
    } else if (nameComps[1] == "utf16be") {
        tf.encoding = TextFormat::Encoding::UTF16_be;
    } else if (nameComps[1] == "win1252") {
        tf.encoding = TextFormat::Encoding::Bytes;
    } else {
        return {false, TextFormat{}};
    }

    if (nameComps[2] == "lf") {
        tf.newLine = TextFormat::NewLine::LF;
    } else if (nameComps[2] == "crlf") {
        tf.newLine = TextFormat::NewLine::CRLF;
    } else {
        return {false, TextFormat{}};
    }

    if (nameComps[3] == "bom") {
        tf.bom = true;
    } else if (nameComps[3] == "nobom") {
        tf.bom = false;
    } else {
        return {false, TextFormat{}};
    }

    return {true, tf};
}

bool runTestSuite() {
    String testsFolder =
        NativePath::join(PLY_WORKSPACE_FOLDER, "repos/plywood/src/apps/AutodetectTest/tests");
    StringWriter sw = StdOut::createStringWriter();
    u32 succeeded = 0;
    u32 failed = 0;
    for (const DirectoryEntry& entry : FileSystem::native()->listDir(testsFolder)) {
        // if (entry.name == "japanese.utf8.lf.bom.txt")
        //    PLY_DEBUG_BREAK();
        if (!entry.isDir && entry.name.endsWith(".txt")) {
            sw << entry.name << "... ";
            Tuple<bool, TextFormat> expected = extractFormatFromName(entry.name.shortenedBy(4));
            if (expected.first) {
                TextFormat detected =
                    FileSystem::native()
                        ->openTextForReadAutodetect(NativePath::join(testsFolder, entry.name))
                        .second;
                if (detected == expected.second) {
                    sw << "OK\n";
                    succeeded++;
                } else {
                    sw << "***failed***\n";
                    failed++;
                }
            } else {
                sw << "***can't parse filename***\n";
                failed++;
            }
            sw.flush();
        }
    }
    sw << "----\n";
    sw.format("{} succeeded, {} failed\n", succeeded, failed);
    return failed == 0;
}

int main() {
    return runTestSuite() ? 0 : 1;
}

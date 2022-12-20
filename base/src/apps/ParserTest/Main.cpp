/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
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

    virtual bool handleError(Owned<cpp::BaseError>&& err) override {
        this->errors.append(std::move(err));
        return true;
    }
};

void runTest(StringView testPath) {
    String testFileContents = FileSystem.loadTextAutodetect(testPath).first;

    // Find the line with five dashes
    ViewInStream vins{testFileContents};
    const char* dashedLine = nullptr;
    for (;;) {
        StringView line = vins.readView<fmt::Line>();
        if (line.isEmpty())
            break;
        if (line.startsWith("-----")) {
            dashedLine = line.bytes;
            break;
        }
    }
    if (!dashedLine) {
        // No dashed line. Consider the whole file source code:
        dashedLine = (const char*) vins.curByte;
    }

    StringView sourceCode = StringView::fromRange(testFileContents.bytes, dashedLine);
    // Note: We could maybe avoid this copy by changing the Preprocessor so that it doesn't always
    // take memory ownership of the input file contents.
    cpp::PPVisitedFiles visitedFiles;
    ParserTestSupervisor visor;
    cpp::grammar::TranslationUnit parseResult = cpp::parse(sourceCode, &visitedFiles, {}, {}, &visor);

    MemOutStream mout;
    mout << sourceCode;
    if (!sourceCode.endsWith("\n")) {
        mout << "\n";
    }
    mout << "-----\n";
    for (const cpp::BaseError* err : visor.errors) {
        err->writeMessage(&mout, &visitedFiles);
    }
    mout.flushMem();
    FileSystem.makeDirsAndSaveTextIfDifferent(testPath, mout.moveToString(), TextFormat::platformPreference());
}

void runTestSuite() {
    String testsFolder =
        Path.join(PLY_WORKSPACE_FOLDER, "repos/plywood/src/apps/ParserTest/tests");
    OutStream outs = StdOut::text();
    for (const DirectoryEntry& entry : FileSystem.listDir(testsFolder)) {
        if (!entry.isDir && entry.name.endsWith(".txt")) {
            outs << entry.name << '\n';
            outs.flushMem();
            runTest(Path.join(testsFolder, entry.name));
        }
    }
}

int main() {
    runTestSuite();
}

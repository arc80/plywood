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
    String testFileContents = FileSystem::native()->loadTextAutodetect(testPath).first;

    // Find the line with five dashes
    StringViewReader strViewReader{testFileContents};
    const char* dashedLine = nullptr;
    for (;;) {
        StringView line = strViewReader.readView<fmt::Line>();
        if (line.isEmpty())
            break;
        if (line.startsWith("-----")) {
            dashedLine = line.bytes;
            break;
        }
    }
    if (!dashedLine) {
        // No dashed line. Consider the whole file source code:
        dashedLine = (const char*) strViewReader.curByte;
    }

    StringView sourceCode = StringView::fromRange(testFileContents.bytes, dashedLine);
    // Note: We could maybe avoid this copy by changing the Preprocessor so that it doesn't always
    // take memory ownership of the input file contents.
    cpp::PPVisitedFiles visitedFiles;
    ParserTestSupervisor visor;
    cpp::grammar::TranslationUnit parseResult = cpp::parse(sourceCode, &visitedFiles, {}, {}, &visor);

    StringWriter sw;
    sw << sourceCode;
    if (!sourceCode.endsWith("\n")) {
        sw << "\n";
    }
    sw << "-----\n";
    for (const cpp::BaseError* err : visor.errors) {
        err->writeMessage(&sw, &visitedFiles);
    }
    sw.flushMem();
    FileSystem::native()->makeDirsAndSaveTextIfDifferent(testPath, sw.moveToString(), TextFormat::platformPreference());
}

void runTestSuite() {
    String testsFolder =
        NativePath::join(PLY_WORKSPACE_FOLDER, "repos/plywood/src/apps/ParserTest/tests");
    StringWriter sw = StdOut::createStringWriter();
    for (const DirectoryEntry& entry : FileSystem::native()->listDir(testsFolder)) {
        if (!entry.isDir && entry.name.endsWith(".txt")) {
            sw << entry.name << '\n';
            sw.flushMem();
            runTest(NativePath::join(testsFolder, entry.name));
        }
    }
}

int main() {
    runTestSuite();
}

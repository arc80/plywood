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

bool errorExportFilter(cpp::PPVisitedFiles* visitedFiles, pylon::Node& aNode, TypedPtr any) {
    if (any.is<cpp::ParseError>()) {
        const cpp::ParseError* error = any.cast<cpp::ParseError>();
        auto objNode = aNode.type.object().switchTo();
        objNode->obj.add("type").value =
            pylon::exportObj(TypedPtr::bind(&error->type), {visitedFiles, errorExportFilter});
        objNode->obj.add("errorToken").value =
            pylon::exportObj(TypedPtr::bind(&error->errorToken), {visitedFiles, errorExportFilter});
        if (error->expected != cpp::ExpectedToken::None) {
            objNode->obj.add("expected").value = pylon::exportObj(
                TypedPtr::bind(&error->expected), {visitedFiles, errorExportFilter});
        }
        if (error->precedingToken.isValid()) {
            objNode->obj.add("precedingToken").value = pylon::exportObj(
                TypedPtr::bind(&error->precedingToken), {visitedFiles, errorExportFilter});
        }
        return true;
    } else if (any.is<cpp::Token>()) {
        const cpp::Token* token = any.cast<cpp::Token>();
        auto objNode = aNode.type.object().switchTo();
        auto valueNode = objNode->obj.add("type").value.type.text().switchTo();
        valueNode->str = TypeResolver<cpp::Token::Type>::get()->findValue(token->type)->name.view();

        auto iter = visitedFiles->locationMap.findLastLessThan(token->linearLoc + 1);
        const cpp::PPVisitedFiles::IncludeChain& chain =
            visitedFiles->includeChains[iter.getItem().includeChainIdx];
        PLY_ASSERT(!chain.isMacroExpansion);
        PLY_ASSERT(chain.fileOrExpIdx == 0);
        const cpp::PPVisitedFiles::SourceFile& srcFile =
            visitedFiles->sourceFiles[chain.fileOrExpIdx];
        FileLocation fileLoc = srcFile.fileLocMap.getFileLocation(
            safeDemote<u32>(token->linearLoc - iter.getItem().offset));
        objNode->obj.add("line").value =
            pylon::Node::createText(String::from(fileLoc.lineNumber), {});
        objNode->obj.add("column").value =
            pylon::Node::createText(String::from(fileLoc.columnNumber), {});
        return true;
    }
    return false;
}

void runTest(StringView testPath) {
    String testFileContents = loadTextFile(testPath);

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
    Tuple<cpp::grammar::TranslationUnit, Array<Owned<cpp::BaseError>>> parseResult =
        cpp::parse(sourceCode, &visitedFiles, {});

    // Convert errors to Pylon
    //    pylon::Node root =
    //        pylon::exportObj(TypedPtr::bind(&parseResult.second), {&visitedFiles,
    //        errorExportFilter});

    MemOutStream mout;
    Owned<StringWriter> sw = TextFormat::platformPreference().createExporter(borrow(&mout));
    *sw << sourceCode;
    if (!sourceCode.endsWith("\n")) {
        *sw << "\n";
    }
    *sw << "-----\n";
    //    pylon::write(sw, root);
    //    *sw << "\n-----\n";
    for (const cpp::BaseError* err : parseResult.second) {
        err->writeMessage(sw, &visitedFiles);
    }
    sw->flushMem();
    FileSystem::native()->makeDirsAndSaveIfDifferent(testPath, mout.moveToBuffer());
}

void runTestSuite() {
    String testsFolder =
        NativePath::join(PLY_WORKSPACE_FOLDER, "repos/plywood/src/apps/ParserTest/tests");
    StringWriter sw{stdOut()};
    for (const DirectoryEntry& entry : FileSystem::native()->listDir(testsFolder)) {
        if (!entry.isDir && entry.name.endsWith(".txt")) {
            // if (!entry.name.startsWith("0063"))
            //    continue;
            sw << entry.name << '\n';
            sw.flushMem();
            runTest(NativePath::join(testsFolder, entry.name));
        }
    }
}

int main() {
    runTestSuite();
}

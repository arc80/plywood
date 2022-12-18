/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <Core.h>
#include <ConsoleUtils.h>
#include <Workspace.h>
#include <ply-cpp/Preprocessor.h>
#include <ply-cpp/ErrorFormatting.h>

void checkFileHeader(StringView srcPath, StringView desiredHeader, const TextFormat& tff) {
    String src = FileSystem::native()->loadTextAutodetect(srcPath).first;
    if (FileSystem::native()->lastResult() != FSResult::OK)
        return;

    using namespace cpp;
    PPVisitedFiles visitedFiles;
    Preprocessor pp;
    pp.visitedFiles = &visitedFiles;

    // FIXME: The following code is duplicated in several places.
    // Factor it out into a convenience function.
    u32 sourceFileIdx = visitedFiles.sourceFiles.numItems();
    PPVisitedFiles::SourceFile& srcFile = visitedFiles.sourceFiles.append();
    srcFile.contents = std::move(src);
    srcFile.fileLocationMap = FileLocationMap::fromView(srcPath, srcFile.contents);

    u32 includeChainIdx = visitedFiles.includeChains.numItems();
    PPVisitedFiles::IncludeChain& includeChain = visitedFiles.includeChains.append();
    includeChain.isMacroExpansion = 0;
    includeChain.fileOrExpIdx = sourceFileIdx;

    Preprocessor::StackItem& item = pp.stack.append();
    item.includeChainIdx = includeChainIdx;
    item.vins = ViewInStream{srcFile.contents};
    pp.linearLocAtEndOfStackTop = srcFile.contents.numBytes;

    PPVisitedFiles::LocationMapTraits::Item locMapItem;
    locMapItem.linearLoc = 0;
    locMapItem.includeChainIdx = includeChainIdx;
    locMapItem.offset = 0;
    visitedFiles.locationMap.insert(std::move(locMapItem));

    bool anyError = false;
    pp.errorHandler = [&](Owned<BaseError>&&) { anyError = true; };
    Token tok;
    for (;;) {
        tok = readToken(&pp);
        if (tok.type != Token::LineComment && tok.type != Token::CStyleComment)
            break;
    }

    ExpandedFileLocation efl = expandFileLocation(&visitedFiles, tok.linearLoc);
    PLY_ASSERT(efl.srcFile == &srcFile);
    StringView existingFileHeader = StringView::fromRange(
        srcFile.contents.bytes, tok.identifier.bytes - efl.fileLoc.numBytesIntoLine);

    // Trim header at first blank line
    {
        ViewInStream vins{existingFileHeader};
        while (StringView line = vins.readView<fmt::Line>()) {
            if (line.rtrim(isWhite).isEmpty()) {
                existingFileHeader = StringView::fromRange(existingFileHeader.bytes, line.bytes);
                break;
            }
        }
    }

    StringView restOfFile =
        StringView::fromRange(existingFileHeader.end(), srcFile.contents.view().end());
    String withHeader = desiredHeader + restOfFile;
    FileSystem::native()->makeDirsAndSaveTextIfDifferent(srcPath, withHeader, tff);
}

void cleanupRepo(StringView repoPath, StringView desiredHeader, StringView clangFormatPath,
                 const TextFormat& tff) {
    for (WalkTriple& triple : FileSystem::native()->walk(repoPath)) {
        for (const WalkTriple::FileInfo& file : triple.files) {
            if (file.name.endsWith(".cpp") || file.name.endsWith(".h")) {
                // Run clang-format
                if (clangFormatPath) {
                    Owned<Subprocess> sub =
                        Subprocess::exec(clangFormatPath, {"-i", file.name}, triple.dirPath,
                                         Subprocess::Output::inherit());
                    if (sub) {
                        sub->join();
                    }
                }

                // Check file header
                checkFileHeader(NativePath::join(triple.dirPath, file.name), desiredHeader, tff);
            }
        }
    }
}

void command_cleanup(CommandLine* cl) {
    ensureTerminated(cl);
    cl->finalize();
    StringView desiredHeader = R"(/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
)";
    cleanupRepo(NativePath::join(Workspace.path, "base/src"), desiredHeader, {},
                Workspace.getSourceTextFormat());
}

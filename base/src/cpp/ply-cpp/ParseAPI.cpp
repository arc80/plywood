/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-cpp/Core.h>
#include <ply-cpp/ParseAPI.h>
#include <ply-cpp/PPVisitedFiles.h>
#include <ply-cpp/Preprocessor.h>
#include <ply-cpp/ParseDeclarations.h>

namespace ply {
namespace cpp {

grammar::TranslationUnit parse(StringView path, String&& sourceCode, PPVisitedFiles* visitedFiles,
                               ArrayView<const PreprocessorDefinition> ppDefs,
                               const Functor<void(StringView directive)>& includeCallback,
                               ParseSupervisor* visor) {
    // Create preprocessor
    Preprocessor pp;
    pp.visitedFiles = visitedFiles;

    // Add sourceCode to array of sourceFiles (note: currently as an unnamed file)
    u32 sourceFileIdx = visitedFiles->sourceFiles.numItems();
    PPVisitedFiles::SourceFile& srcFile = visitedFiles->sourceFiles.append();
    srcFile.contents = std::move(sourceCode);
    srcFile.fileLocationMap = FileLocationMap::fromView(path, srcFile.contents);

    // Create include chain for this file
    u32 includeChainIdx = visitedFiles->includeChains.numItems();
    PPVisitedFiles::IncludeChain& includeChain = visitedFiles->includeChains.append();
    includeChain.isMacroExpansion = 0;
    includeChain.fileOrExpIdx = sourceFileIdx;

    // Create preprocessor stack
    Preprocessor::StackItem& item = pp.stack.append();
    item.includeChainIdx = includeChainIdx;
    item.vins = ViewInStream{srcFile.contents};
    pp.linearLocAtEndOfStackTop = srcFile.contents.numBytes;

    // Initialize location map
    PPVisitedFiles::LocationMapTraits::Item locMapItem;
    locMapItem.linearLoc = 0;
    locMapItem.includeChainIdx = includeChainIdx;
    locMapItem.offset = 0;
    visitedFiles->locationMap.insert(std::move(locMapItem));

    for (const PreprocessorDefinition& ppDef : ppDefs) {
        addPPDef(&pp, ppDef.identifier, ppDef.value, ppDef.takesArgs);
    }

    // Create parser
    Parser parser;
    parser.pp = &pp;
    ParseSupervisor emptyVisor;
    parser.visor = visor ? visor : &emptyVisor;

    pp.errorHandler = [&](Owned<BaseError>&& err) { visor->handleError(std::move(err)); };

    // Do parse
    grammar::TranslationUnit tu = parseTranslationUnit(&parser);
    return tu;
}

Tuple<grammar::Declaration::Simple, Array<Owned<BaseError>>>
parseSimpleDeclaration(StringView sourceCode, LinearLocation linearLocOfs) {
    // Create preprocessor
    PPVisitedFiles visitedFiles;
    Preprocessor pp;
    pp.visitedFiles = &visitedFiles;

    // Add sourceCode to array of sourceFiles an unnamed file
    u32 sourceFileIdx = visitedFiles.sourceFiles.numItems();
    PPVisitedFiles::SourceFile& srcFile = visitedFiles.sourceFiles.append();
    srcFile.contents = sourceCode;

    // Create include chain for this file
    u32 includeChainIdx = visitedFiles.includeChains.numItems();
    PPVisitedFiles::IncludeChain& includeChain = visitedFiles.includeChains.append();
    includeChain.isMacroExpansion = 0;
    includeChain.fileOrExpIdx = sourceFileIdx;

    // Create preprocessor stack
    Preprocessor::StackItem& item = pp.stack.append();
    item.includeChainIdx = includeChainIdx;
    item.vins = ViewInStream{srcFile.contents};
    pp.linearLocAtEndOfStackTop = linearLocOfs + srcFile.contents.numBytes;

    // Create parser
    Parser parser;
    parser.pp = &pp;
    ParseSupervisor emptyVisor;
    parser.visor = &emptyVisor;

    Array<Owned<BaseError>> errors;
    pp.errorHandler = [&](Owned<BaseError>&& err) { errors.append(std::move(err)); };

    // Do parse
    grammar::Declaration::Simple simple;
    parseSpecifiersAndDeclarators(&parser, simple, {SpecDcorMode::GlobalOrMember});
    return {std::move(simple), std::move(errors)};
}

} // namespace cpp
} // namespace ply

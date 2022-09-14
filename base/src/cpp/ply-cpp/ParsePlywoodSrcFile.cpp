/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-cpp/Core.h>
#include <ply-cpp/Parser.h>
#include <ply-cpp/Preprocessor.h>
#include <ply-cpp/ErrorFormatting.h>

namespace ply {
namespace cpp {

void parsePlywoodSrcFile(StringView absSrcPath, cpp::PPVisitedFiles* visitedFiles,
                         ParseSupervisor* visor) {
    Preprocessor pp;
    pp.visitedFiles = visitedFiles;

    u32 sourceFileIdx = visitedFiles->sourceFiles.numItems();
    PPVisitedFiles::SourceFile& srcFile = visitedFiles->sourceFiles.append();
    String src = FileSystem::native()->loadTextAutodetect(absSrcPath).first;
    if (FileSystem::native()->lastResult() != FSResult::OK) {
        struct ErrorWrapper : BaseError {
            String msg;
            PLY_INLINE ErrorWrapper(String&& msg) : msg{std::move(msg)} {
            }
            void writeMessage(OutStream* outs, const PPVisitedFiles*) const override {
                *outs << msg;
            }
        };
        visor->handleError(new ErrorWrapper{String::format("Can't open '{}'\n", absSrcPath)});
        return;
    }
    srcFile.contents = std::move(src);
    srcFile.fileLocationMap = FileLocationMap::fromView(absSrcPath, srcFile.contents);

    u32 includeChainIdx = visitedFiles->includeChains.numItems();
    PPVisitedFiles::IncludeChain& includeChain = visitedFiles->includeChains.append();
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
    visitedFiles->locationMap.insert(std::move(locMapItem));

    addPPDef(&pp, "PLY_INLINE", "");
    addPPDef(&pp, "PLY_NO_INLINE", "");
    addPPDef(&pp, "PLY_NO_DISCARD", "");
    addPPDef(&pp, "PLY_DLL_ENTRY", "");
    addPPDef(&pp, "PLY_BUILD_ENTRY", "");
    addPPDef(&pp, "PYLON_ENTRY", "");
    addPPDef(&pp, "PLY_STATIC_ASSERT", "static_assert");
    addPPDef(&pp, "PLY_STATE_REFLECT", "", true);
    addPPDef(&pp, "PLY_REFLECT", "", true);
    addPPDef(&pp, "PLY_REFLECT_ENUM", "", true);
    addPPDef(&pp, "PLY_IMPLEMENT_IFACE", "", true);
    addPPDef(&pp, "PLY_STRUCT_BEGIN", "", true);
    addPPDef(&pp, "PLY_STRUCT_BEGIN_PRIM", "", true);
    addPPDef(&pp, "PLY_STRUCT_BEGIN_PRIM_NO_IMPORT", "", true);
    addPPDef(&pp, "PLY_STRUCT_END", "", true);
    addPPDef(&pp, "PLY_STRUCT_END_PRIM", "", true);
    addPPDef(&pp, "PLY_STRUCT_MEMBER", "", true);
    addPPDef(&pp, "PLY_ENUM_BEGIN", "", true);
    addPPDef(&pp, "PLY_ENUM_IDENTIFIER", "", true);
    addPPDef(&pp, "PLY_ENUM_END", "", true);
    addPPDef(&pp, "PLY_STATE", "", true);
    addPPDef(&pp, "PLY_IFACE_METHOD", "", true);
    addPPDef(&pp, "IMP_FUNC", "", true);
    addPPDef(&pp, "SLOG_CHANNEL", "", true);
    addPPDef(&pp, "SLOG_NO_CHANNEL", "", true);
    addPPDef(&pp, "SLOG_DECLARE_CHANNEL", "", true);
    addPPDef(&pp, "PLY_WORKSPACE_FOLDER", "\"\"", false);
    addPPDef(&pp, "PLY_THREAD_STARTCALL", "", false);
    addPPDef(&pp, "GL_FUNC", "", true);
    addPPDef(&pp, "PLY_MAKE_LIMITS", "", true);
    addPPDef(&pp, "PLY_DECL_ALIGNED", "", true);
    addPPDef(&pp, "WINAPI", "", false);
    addPPDef(&pp, "APIENTRY", "", false);
    addPPDef(&pp, "PLY_MAKE_WELL_FORMEDNESS_CHECK_1", "", true);
    addPPDef(&pp, "PLY_MAKE_WELL_FORMEDNESS_CHECK_2", "", true);
    addPPDef(&pp, "PLY_BIND_METHOD", "", true);
    addPPDef(&pp, "PLY_DECLARE_TYPE_DESCRIPTOR", "", true);
    addPPDef(&pp, "PLY_DEFINE_TYPE_DESCRIPTOR", "void foo", false);
    addPPDef(&pp, "PLY_METHOD_TABLES_ONLY", "", true);
    addPPDef(&pp, "SWITCH_FOOTER", "", true);   // temporary
    addPPDef(&pp, "SWITCH_ACCESSOR", "", true); // temporary
    addPPDef(&pp, "PLY_TEST_CASE", "void foo()", true);
    addPPDef(&pp, "PLY_DEFINE_RACE_DETECTOR", "", true);
    Parser parser;
    parser.pp = &pp;
    pp.includeCallback = [visor](StringView directive) { visor->onGotInclude(directive); };
    parser.visor = visor;
    PLY_ASSERT(!visor->parser);
    visor->parser = &parser;

    pp.errorHandler = [&](Owned<BaseError>&& err) { visor->handleError(std::move(err)); };

    grammar::TranslationUnit tu = parseTranslationUnit(&parser);

    visor->parser = nullptr;
}

} // namespace cpp
} // namespace ply

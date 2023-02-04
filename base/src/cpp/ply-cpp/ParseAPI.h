/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once
#include <ply-cpp/Core.h>
#include <ply-cpp/Parser.h>
#include <ply-cpp/PPVisitedFiles.h>
#include <ply-cpp/Error.h>

namespace ply {
namespace cpp {

struct PreprocessorDefinition {
    String identifier;
    String value;
    bool takesArgs = false;
};

grammar::TranslationUnit parse(StringView path, String&& sourceCode, PPVisitedFiles* visitedFiles,
                               ArrayView<const PreprocessorDefinition> ppDefs = {},
                               const Func<void(StringView directive)>& includeCallback = {},
                               ParseSupervisor* visor = nullptr);

Tuple<grammar::Declaration::Simple, Array<Owned<BaseError>>>
parseSimpleDeclaration(StringView sourceCode, LinearLocation linearLocOfs = 0);

} // namespace cpp
} // namespace ply

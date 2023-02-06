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
    bool takes_args = false;
};

grammar::TranslationUnit
parse(StringView path, String&& source_code, PPVisitedFiles* visited_files,
      ArrayView<const PreprocessorDefinition> pp_defs = {},
      const Func<void(StringView directive)>& include_callback = {},
      ParseSupervisor* visor = nullptr);

Tuple<grammar::Declaration::Simple, Array<Owned<BaseError>>>
parse_simple_declaration(StringView source_code, LinearLocation linear_loc_ofs = 0);

} // namespace cpp
} // namespace ply

/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-cpp/Core.h>
#include <ply-cpp/ParseAPI.h>
#include <ply-cpp/PPVisitedFiles.h>
#include <ply-cpp/Preprocessor.h>
#include <ply-cpp/ParseDeclarations.h>

namespace ply {
namespace cpp {

grammar::TranslationUnit parse(StringView path, String&& source_code,
                               PPVisitedFiles* visited_files,
                               ArrayView<const PreprocessorDefinition> pp_defs,
                               const Func<void(StringView directive)>& include_callback,
                               ParseSupervisor* visor) {
    // Create preprocessor
    Preprocessor pp;
    pp.visited_files = visited_files;

    // Add source_code to array of source_files (note: currently as an unnamed file)
    u32 source_file_idx = visited_files->source_files.num_items();
    PPVisitedFiles::SourceFile& src_file = visited_files->source_files.append();
    src_file.contents = std::move(source_code);
    src_file.file_location_map = FileLocationMap::from_view(path, src_file.contents);

    // Create include chain for this file
    u32 include_chain_idx = visited_files->include_chains.num_items();
    PPVisitedFiles::IncludeChain& include_chain =
        visited_files->include_chains.append();
    include_chain.is_macro_expansion = 0;
    include_chain.file_or_exp_idx = source_file_idx;

    // Create preprocessor stack
    Preprocessor::StackItem& item = pp.stack.append();
    item.include_chain_idx = include_chain_idx;
    item.in = ViewInStream{src_file.contents};
    pp.linear_loc_at_end_of_stack_top = src_file.contents.num_bytes;

    // Initialize location map
    PPVisitedFiles::LocationMapTraits::Item loc_map_item;
    loc_map_item.linear_loc = 0;
    loc_map_item.include_chain_idx = include_chain_idx;
    loc_map_item.offset = 0;
    visited_files->location_map.insert(std::move(loc_map_item));

    for (const PreprocessorDefinition& pp_def : pp_defs) {
        add_ppdef(&pp, pp_def.identifier, pp_def.value, pp_def.takes_args);
    }

    // Create parser
    Parser parser;
    parser.pp = &pp;
    ParseSupervisor empty_visor;
    parser.visor = visor ? visor : &empty_visor;

    pp.error_handler = [&](Owned<BaseError>&& err) {
        visor->handle_error(std::move(err));
    };

    // Do parse
    grammar::TranslationUnit tu = parse_translation_unit(&parser);
    return tu;
}

Tuple<grammar::Declaration::Simple, Array<Owned<BaseError>>>
parse_simple_declaration(StringView source_code, LinearLocation linear_loc_ofs) {
    // Create preprocessor
    PPVisitedFiles visited_files;
    Preprocessor pp;
    pp.visited_files = &visited_files;

    // Add source_code to array of source_files an unnamed file
    u32 source_file_idx = visited_files.source_files.num_items();
    PPVisitedFiles::SourceFile& src_file = visited_files.source_files.append();
    src_file.contents = source_code;

    // Create include chain for this file
    u32 include_chain_idx = visited_files.include_chains.num_items();
    PPVisitedFiles::IncludeChain& include_chain = visited_files.include_chains.append();
    include_chain.is_macro_expansion = 0;
    include_chain.file_or_exp_idx = source_file_idx;

    // Create preprocessor stack
    Preprocessor::StackItem& item = pp.stack.append();
    item.include_chain_idx = include_chain_idx;
    item.in = ViewInStream{src_file.contents};
    pp.linear_loc_at_end_of_stack_top = linear_loc_ofs + src_file.contents.num_bytes;

    // Create parser
    Parser parser;
    parser.pp = &pp;
    ParseSupervisor empty_visor;
    parser.visor = &empty_visor;

    Array<Owned<BaseError>> errors;
    pp.error_handler = [&](Owned<BaseError>&& err) { errors.append(std::move(err)); };

    // Do parse
    grammar::Declaration::Simple simple;
    parse_specifiers_and_declarators(&parser, simple, {SpecDcorMode::GlobalOrMember});
    return {std::move(simple), std::move(errors)};
}

} // namespace cpp
} // namespace ply

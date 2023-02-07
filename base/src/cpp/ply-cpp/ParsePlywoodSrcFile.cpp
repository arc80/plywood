/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-cpp/Core.h>
#include <ply-cpp/Parser.h>
#include <ply-cpp/Preprocessor.h>
#include <ply-cpp/ErrorFormatting.h>

namespace ply {
namespace cpp {

void parse_plywood_src_file(StringView abs_src_path, cpp::PPVisitedFiles* visited_files,
                            ParseSupervisor* visor) {
    Preprocessor pp;
    pp.visited_files = visited_files;

    u32 source_file_idx = visited_files->source_files.num_items();
    PPVisitedFiles::SourceFile& src_file = visited_files->source_files.append();
    String src = FileSystem.load_text_autodetect(abs_src_path);
    if (FileSystem.last_result() != FSResult::OK) {
        struct ErrorWrapper : BaseError {
            String msg;
            ErrorWrapper(String&& msg) : msg{std::move(msg)} {
            }
            void write_message(OutStream& out, const PPVisitedFiles*) const override {
                out << msg;
            }
        };
        visor->handle_error(
            new ErrorWrapper{String::format("Can't open '{}'\n", abs_src_path)});
        return;
    }
    src_file.contents = std::move(src);
    src_file.file_location_map =
        FileLocationMap::from_view(abs_src_path, src_file.contents);

    u32 include_chain_idx = visited_files->include_chains.num_items();
    PPVisitedFiles::IncludeChain& include_chain =
        visited_files->include_chains.append();
    include_chain.is_macro_expansion = 0;
    include_chain.file_or_exp_idx = source_file_idx;

    Preprocessor::StackItem& item = pp.stack.append();
    item.include_chain_idx = include_chain_idx;
    item.in = ViewInStream{src_file.contents};
    pp.linear_loc_at_end_of_stack_top = src_file.contents.num_bytes;

    PPVisitedFiles::LocationMapTraits::Item loc_map_item;
    loc_map_item.linear_loc = 0;
    loc_map_item.include_chain_idx = include_chain_idx;
    loc_map_item.offset = 0;
    visited_files->location_map.insert(std::move(loc_map_item));

    add_ppdef(&pp, "PLY_INLINE", "");
    add_ppdef(&pp, "PLY_NO_INLINE", "");
    add_ppdef(&pp, "PLY_NO_DISCARD", "");
    add_ppdef(&pp, "PLY_BUILD_ENTRY", "");
    add_ppdef(&pp, "PYLON_ENTRY", "");
    add_ppdef(&pp, "PLY_STATIC_ASSERT", "static_assert");
    add_ppdef(&pp, "PLY_STATE_REFLECT", "", true);
    add_ppdef(&pp, "PLY_REFLECT", "", true);
    add_ppdef(&pp, "PLY_REFLECT_ENUM", "", true);
    add_ppdef(&pp, "PLY_IMPLEMENT_IFACE", "", true);
    add_ppdef(&pp, "PLY_STRUCT_BEGIN", "", true);
    add_ppdef(&pp, "PLY_STRUCT_BEGIN_PRIM", "", true);
    add_ppdef(&pp, "PLY_STRUCT_BEGIN_PRIM_NO_IMPORT", "", true);
    add_ppdef(&pp, "PLY_STRUCT_END", "", true);
    add_ppdef(&pp, "PLY_STRUCT_END_PRIM", "", true);
    add_ppdef(&pp, "PLY_STRUCT_MEMBER", "", true);
    add_ppdef(&pp, "PLY_ENUM_BEGIN", "", true);
    add_ppdef(&pp, "PLY_ENUM_IDENTIFIER", "", true);
    add_ppdef(&pp, "PLY_ENUM_END", "", true);
    add_ppdef(&pp, "PLY_STATE", "", true);
    add_ppdef(&pp, "PLY_IFACE_METHOD", "", true);
    add_ppdef(&pp, "IMP_FUNC", "", true);
    add_ppdef(&pp, "SLOG_CHANNEL", "", true);
    add_ppdef(&pp, "SLOG_NO_CHANNEL", "", true);
    add_ppdef(&pp, "SLOG_DECLARE_CHANNEL", "", true);
    add_ppdef(&pp, "PLY_WORKSPACE_FOLDER", "\"\"", false);
    add_ppdef(&pp, "PLY_THREAD_STARTCALL", "", false);
    add_ppdef(&pp, "GL_FUNC", "", true);
    add_ppdef(&pp, "PLY_MAKE_LIMITS", "", true);
    add_ppdef(&pp, "PLY_DECL_ALIGNED", "", true);
    add_ppdef(&pp, "WINAPI", "", false);
    add_ppdef(&pp, "APIENTRY", "", false);
    add_ppdef(&pp, "PLY_MAKE_WELL_FORMEDNESS_CHECK_1", "", true);
    add_ppdef(&pp, "PLY_MAKE_WELL_FORMEDNESS_CHECK_2", "", true);
    add_ppdef(&pp, "PLY_BIND_METHOD", "", true);
    add_ppdef(&pp, "PLY_DECLARE_TYPE_DESCRIPTOR", "", true);
    add_ppdef(&pp, "PLY_DEFINE_TYPE_DESCRIPTOR", "void foo", false);
    add_ppdef(&pp, "PLY_METHOD_TABLES_ONLY", "", true);
    add_ppdef(&pp, "SWITCH_FOOTER", "", true);   // temporary
    add_ppdef(&pp, "SWITCH_ACCESSOR", "", true); // temporary
    add_ppdef(&pp, "PLY_TEST_CASE", "void foo()", true);
    add_ppdef(&pp, "PLY_DEFINE_RACE_DETECTOR", "", true);
    Parser parser;
    parser.pp = &pp;
    pp.include_callback = [visor](StringView directive) {
        visor->on_got_include(directive);
    };
    parser.visor = visor;
    PLY_ASSERT(!visor->parser);
    visor->parser = &parser;

    pp.error_handler = [&](Owned<BaseError>&& err) {
        visor->handle_error(std::move(err));
    };

    grammar::TranslationUnit tu = parse_translation_unit(&parser);

    visor->parser = nullptr;
}

} // namespace cpp
} // namespace ply

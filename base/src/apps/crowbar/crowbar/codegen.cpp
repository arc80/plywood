/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include "core.h"
#include <ply-build-repo/Workspace.h>
#include <ply-cpp/Parser.h>
#include <ply-cpp/Preprocessor.h>
#include <ply-cpp/PPVisitedFiles.h>
#include <ply-cpp/ErrorFormatting.h>

//                              ▄▄
//  ▄▄▄▄▄   ▄▄▄▄  ▄▄▄▄▄   ▄▄▄▄  ▄▄ ▄▄▄▄▄   ▄▄▄▄▄
//  ██  ██  ▄▄▄██ ██  ▀▀ ▀█▄▄▄  ██ ██  ██ ██  ██
//  ██▄▄█▀ ▀█▄▄██ ██      ▄▄▄█▀ ██ ██  ██ ▀█▄▄██
//  ██                                     ▄▄▄█▀

struct Subst {
    u32 start = 0;
    u32 num_bytes = 0;
    String replacement;

    bool operator<(const Subst& other) const {
        return (this->start < other.start) ||
               (this->start == other.start && this->num_bytes < other.num_bytes);
    }
};

struct ReflectedClass {
    PLY_REFLECT()
    String cpp_inl_path;
    String name;
    Array<String> members;
    // ply reflect off
};

// FIXME: The parser actually fills in Enum_::enumerators, making this struct redundant.
// Find a way to simplify.
struct ReflectedEnum {
    PLY_REFLECT()
    String cpp_inl_path;
    String namespace_prefix;
    String enum_name;
    Array<String> enumerators;
    // ply reflect off
};

struct SwitchInfo {
    PLY_REFLECT()
    cpp::Token macro;
    String inline_inl_path;
    String cpp_inl_path;
    String name;
    bool is_reflected = false;
    Array<String> states;
    // ply reflect off
};

struct ReflectionInfoAggregator {
    Array<Owned<ReflectedClass>> classes;
    Array<Owned<ReflectedEnum>> enums;
    Array<Owned<SwitchInfo>> switches;
};

struct SingleFileReflectionInfo {
    // May later need to generalize to multiple files
    Array<Subst> substs_in_parsed_file;
    Array<SwitchInfo*> switches;
};

namespace ply {
namespace cpp {
void parse_plywood_src_file(StringView abs_src_path, cpp::PPVisitedFiles* visited_files,
                            cpp::ParseSupervisor* visor);
} // namespace cpp
} // namespace ply

// FIXME: Just call this once per parsed file
String make_inl_rel_path(StringView rel_path) {
    auto split_path = Path.split(rel_path);
    return Path.join(split_path.first, "codegen",
                     Path.split_ext(split_path.second).first + ".inl");
}

Subst insert_directive_subst(StringView view, const char* marker,
                             String&& replacement) {
    Subst subst;
    subst.replacement = std::move(replacement);
    const char* insert_pos = marker;
    while (view.bytes < insert_pos) {
        char c = insert_pos[-1];
        if (c == '\n') {
            break;
        } else if (!is_white(c)) {
            const char* first_non_white = insert_pos - 1;
            const char* line_start = first_non_white;
            while (view.bytes < line_start) {
                c = line_start[-1];
                if (c == '\n')
                    break;
                line_start--;
                if (!is_white(c)) {
                    first_non_white = line_start;
                }
            }
            subst.replacement = String::format(
                "\n{}{}", subst.replacement,
                StringView{line_start, check_cast<u32>(first_non_white - line_start)});
            break;
        }
        insert_pos--;
    }
    subst.start = check_cast<u32>(insert_pos - view.bytes);
    subst.num_bytes = 0;
    return subst;
}

struct ReflectionHookError : cpp::BaseError {
    enum Type {
        // ply reflect enum
        Unknown,
        SwitchMayOnlyContainStructs,
        MissingReflectOffCommand,
        UnexpectedReflectOffCommand,
        CannotInjectCodeIntoMacro,
        DuplicateCommand,
        CommandCanOnlyBeUsedAtDeclarationScope,
        CommandCanOnlyBeUsedInClassOrStruct,
        CommandCanOnlyBeUsedInsideEnum,
        UnrecognizedCommand,
    };

    PLY_REFLECT()
    Type type = Unknown;
    cpp::LinearLocation linear_loc = -1;
    cpp::LinearLocation other_loc = -1;
    // ply reflect off

    ReflectionHookError(Type type, cpp::LinearLocation linear_loc,
                        cpp::LinearLocation other_loc = -1)
        : type{type}, linear_loc{linear_loc}, other_loc{other_loc} {
    }
    virtual void write_message(OutStream& out,
                               const cpp::PPVisitedFiles* visited_files) const override;
};

PLY_DECLARE_TYPE_DESCRIPTOR(ReflectionHookError::Type)

void ReflectionHookError::write_message(
    OutStream& out, const cpp::PPVisitedFiles* visited_files) const {
    out.format("{}: error: ",
               expand_file_location(visited_files, this->linear_loc).to_string());
    switch (this->type) {
        case ReflectionHookError::SwitchMayOnlyContainStructs: {
            out << "a switch may only contain structs\n";
            break;
        }
        case ReflectionHookError::MissingReflectOffCommand: {
            out << "can't find matching // ply reflect off\n";
            break;
        }
        case ReflectionHookError::UnexpectedReflectOffCommand: {
            out << "unexpected // ply reflect off\n";
            break;
        }
        case ReflectionHookError::CannotInjectCodeIntoMacro: {
            out << "can't inject code inside macro\n";
            out.format(
                "{}: note: for code injected by this command\n",
                expand_file_location(visited_files, this->other_loc).to_string());
            break;
        }
        case ReflectionHookError::DuplicateCommand: {
            out << "duplicate command\n";
            out.format(
                "{}: note: see previous command\n",
                expand_file_location(visited_files, this->other_loc).to_string());
            break;
        }
        case ReflectionHookError::CommandCanOnlyBeUsedAtDeclarationScope: {
            out << "command can only be used at declaration scope\n";
            break;
        }
        case ReflectionHookError::CommandCanOnlyBeUsedInClassOrStruct: {
            out << "command can only be used inside a class or struct\n";
            break;
        }
        case ReflectionHookError::CommandCanOnlyBeUsedInsideEnum: {
            out << "command can only be used inside an enum\n";
            break;
        }
        case ReflectionHookError::UnrecognizedCommand: {
            out << "unrecognized command\n";
            break;
        }
        default: {
            out << "error message not implemented!\n";
            break;
        }
    }
}

struct ReflectionHooks : cpp::ParseSupervisor {
    struct State {
        ReflectedClass* clazz = nullptr;
        cpp::Token capture_members_token;
        Owned<SwitchInfo> switch_ = nullptr;
        Owned<ReflectedEnum> enum_ = nullptr;
        cpp::Token keep_reflected_enum_token;
    };
    Array<State> stack;
    StringView file_path;
    ReflectionInfoAggregator* agg = nullptr;
    SingleFileReflectionInfo* sfri = nullptr;
    bool any_error = false;

    virtual void enter(AnyObject node) override {
        State& state = this->stack.append();

        if (node.is<cpp::grammar::DeclSpecifier::Enum_>()) {
            state.enum_ = new ReflectedEnum;
            state.enum_->cpp_inl_path = make_inl_rel_path(this->file_path);
            state.enum_->namespace_prefix = this->get_namespace_prefix();
            state.enum_->enum_name = this->get_class_name("::", false);
            return;
        }

        if (this->stack.num_items() > 1) {
            State& parent_state = this->stack.back(-2);

            if (parent_state.switch_) {
                auto* record = node.safe_cast<cpp::grammar::DeclSpecifier::Record>();
                if (!record || record->class_key.identifier == "union") {
                    this->parser->pp->error_handler(new ReflectionHookError{
                        ReflectionHookError::SwitchMayOnlyContainStructs,
                        parent_state.switch_->macro.linear_loc});
                    return;
                }
                parent_state.switch_->states.append(record->qid.to_string());
                return;
            }
        }
    }

    virtual void exit(AnyObject node) override {
        State& state = this->stack.back();
        if (state.capture_members_token.is_valid()) {
            this->parser->pp->error_handler(
                new ReflectionHookError{ReflectionHookError::MissingReflectOffCommand,
                                        state.capture_members_token.linear_loc});
        }

        if (state.keep_reflected_enum_token.is_valid()) {
            this->agg->enums.append(std::move(state.enum_));
        }

        if (state.switch_) {
            auto* record = node.safe_cast<cpp::grammar::DeclSpecifier::Record>();
            PLY_ASSERT(record); // guaranteed by enter()
            PLY_ASSERT(record->class_key.identifier !=
                       "union"); // guaranteed by enter()
            if (record->close_curly.is_valid()) {
                cpp::PPVisitedFiles* vf = this->parser->pp->visited_files;
                auto iter = vf->location_map.find_last_less_than(
                    record->close_curly.linear_loc + 1);
                const cpp::PPVisitedFiles::LocationMapTraits::Item& lm_item =
                    iter.get_item();
                const cpp::PPVisitedFiles::IncludeChain& chain =
                    vf->include_chains[lm_item.include_chain_idx];
                if (chain.is_macro_expansion) {
                    this->parser->pp->error_handler(new ReflectionHookError{
                        ReflectionHookError::CannotInjectCodeIntoMacro,
                        record->close_curly.linear_loc,
                        state.switch_->macro.linear_loc});
                } else {
                    const cpp::PPVisitedFiles::SourceFile& src_file =
                        vf->source_files[chain.file_or_exp_idx];
                    String cur_abs_path = Path.join(Workspace.path, this->file_path);
                    // FIXME: Improve this if we ever start following includes while
                    // collecting reflection info:
                    PLY_ASSERT(src_file.file_location_map.path == cur_abs_path);
                    const char* end_curly =
                        src_file.contents.bytes +
                        (lm_item.offset + record->close_curly.linear_loc -
                         lm_item.linear_loc);
                    PLY_ASSERT(*end_curly == '}');
                    String gen_file_name =
                        String::format("switch-{}.inl", this->get_class_name("-"));
                    state.switch_->inline_inl_path = Path.join(
                        Path.split(this->file_path).first, "codegen", gen_file_name);
                    this->sfri->substs_in_parsed_file.append(insert_directive_subst(
                        src_file.contents, end_curly,
                        String::format("#include \"codegen/{}\" //@@ply\n",
                                       gen_file_name)));

                    // Add section(s) to .cpp .inl
                    this->sfri->switches.append(state.switch_);
                    this->agg->switches.append(std::move(state.switch_));
                }
            }
        }
        stack.pop();
    }

    virtual void on_got_declaration(const cpp::grammar::Declaration& decl) override {
        State& state = this->stack.back();
        if (state.capture_members_token.is_valid()) {
            if (auto simple = decl.simple()) {
                if (!simple->init_declarators.is_empty()) {
                    const cpp::grammar::InitDeclaratorWithComma& init_decl =
                        simple->init_declarators[0];
                    if (!init_decl.dcor.is_function()) {
                        state.clazz->members.append(init_decl.dcor.qid.to_string());
                    }
                }
            }
        }
    }

    virtual void
    on_got_enumerator(const cpp::grammar::InitEnumeratorWithComma* init_enor) override {
        State& state = this->stack.back();
        PLY_ASSERT(state.enum_);
        state.enum_->enumerators.append(init_enor->identifier.identifier);
    }

    virtual void on_got_include(StringView directive) override {
        const cpp::Preprocessor::StackItem& pp_item = this->parser->pp->stack.back();
        const char* pp_item_start_unit = (const char*) pp_item.in.cur_byte;
        PLY_ASSERT(directive.bytes >= pp_item_start_unit &&
                   directive.end() <= (const char*) pp_item.in.end_byte);
        if (!directive.rtrim([](char c) { return is_white(c); }).ends_with("//@@ply"))
            return;

        // This will delete the line containing the #include:
        const char* start_of_line = directive.bytes;
        while ((start_of_line > pp_item_start_unit) && (start_of_line[-1] != '\n')) {
            start_of_line--;
        }
        Subst& subst = this->sfri->substs_in_parsed_file.append();
        subst.start = check_cast<u32>(start_of_line - pp_item_start_unit);
        subst.num_bytes =
            check_cast<u32>((directive.bytes + directive.num_bytes) - start_of_line);
    }

    void begin_capture(const cpp::Token& token) {
        State& state = this->stack.back();
        if (state.clazz) {
            // Already have a PLY_REFLECT macro
            this->parser->pp->error_handler(new ReflectionHookError{
                ReflectionHookError::DuplicateCommand, token.linear_loc,
                state.capture_members_token.linear_loc});
            return;
        }
        ReflectedClass* clazz = new ReflectedClass;
        clazz->cpp_inl_path = make_inl_rel_path(this->file_path);
        clazz->name = this->get_class_name();
        this->agg->classes.append(clazz);
        state.clazz = clazz;
        state.capture_members_token = token;
    }

    virtual void got_macro_or_comment(cpp::Token token) override {
        if (token.type == cpp::Token::Macro) {
            if (token.identifier == "PLY_STATE_REFLECT" ||
                token.identifier == "PLY_REFLECT") {
                if (!this->parser->at_declaration_scope) {
                    this->parser->pp->error_handler(new ReflectionHookError{
                        ReflectionHookError::CommandCanOnlyBeUsedAtDeclarationScope,
                        token.linear_loc});
                    return;
                }
                auto* record = this->scope_stack.back()
                                   .safe_cast<cpp::grammar::DeclSpecifier::Record>();
                if (!record || record->class_key.identifier == "union") {
                    this->parser->pp->error_handler(new ReflectionHookError{
                        ReflectionHookError::CommandCanOnlyBeUsedInClassOrStruct,
                        token.linear_loc});
                    return;
                }
                this->begin_capture(token);
            }
        } else if (token.type == cpp::Token::LineComment) {
            ViewInStream comment_reader{token.identifier};
            PLY_ASSERT(comment_reader.view_readable().starts_with("//"));
            comment_reader.cur_byte += 2;
            comment_reader.parse<fmt::Whitespace>();
            if (comment_reader.view_readable().starts_with("%%")) {
                comment_reader.cur_byte += 2;
                comment_reader.parse<fmt::Whitespace>();
                StringView cmd = comment_reader.read_view<fmt::Identifier>();
                if (cmd == "end") {
                    if (this->parser->at_declaration_scope) {
                        State& state = this->stack.back();
                        if (!state.capture_members_token.is_valid()) {
                            // this->parser->error(); // Not capturing
                            return;
                        }
                        state.capture_members_token = {};
                    }
                }
            } else if (comment_reader.view_readable().starts_with("ply ")) {
                String fixed = String::format(
                    "// {}\n",
                    StringView{" "}.join(comment_reader.view_readable()
                                             .rtrim([](char c) { return is_white(c); })
                                             .split_byte(' ')));
                if (fixed == "// ply make switch\n" ||
                    fixed == "// ply make reflected switch\n") {
                    // This is a switch
                    if (!this->parser->at_declaration_scope) {
                        this->parser->pp->error_handler(new ReflectionHookError{
                            ReflectionHookError::CommandCanOnlyBeUsedAtDeclarationScope,
                            token.linear_loc});
                        return;
                    }
                    auto* record =
                        this->scope_stack.back()
                            .safe_cast<cpp::grammar::DeclSpecifier::Record>();
                    if (!record || record->class_key.identifier == "union") {
                        this->parser->pp->error_handler(new ReflectionHookError{
                            ReflectionHookError::CommandCanOnlyBeUsedInClassOrStruct,
                            token.linear_loc});
                        return;
                    }
                    State& state = this->stack.back();
                    if (state.switch_) {
                        this->parser->pp->error_handler(new ReflectionHookError{
                            ReflectionHookError::DuplicateCommand, token.linear_loc,
                            state.switch_->macro.linear_loc});
                        return;
                    }
                    SwitchInfo* switch_ = new SwitchInfo;
                    switch_->macro = token;
                    switch_->cpp_inl_path = make_inl_rel_path(this->file_path);
                    switch_->name = this->get_class_name();
                    state.switch_ = switch_;
                    if (fixed == "// ply make reflected switch\n") {
                        switch_->is_reflected = true;
                    }
                } else if (fixed == "// ply reflect off\n") {
                    if (!this->parser->at_declaration_scope) {
                        this->parser->pp->error_handler(new ReflectionHookError{
                            ReflectionHookError::CommandCanOnlyBeUsedAtDeclarationScope,
                            token.linear_loc});
                        return;
                    }
                    State& state = this->stack.back();
                    if (!state.capture_members_token.is_valid()) {
                        this->parser->pp->error_handler(new ReflectionHookError{
                            ReflectionHookError::UnexpectedReflectOffCommand,
                            token.linear_loc});
                        return;
                    }
                    state.capture_members_token = {};
                } else if (fixed == "// ply reflect enum\n") {
                    auto* enum_ = this->scope_stack.back()
                                      .safe_cast<cpp::grammar::DeclSpecifier::Enum_>();
                    if (!enum_) {
                        this->parser->pp->error_handler(new ReflectionHookError{
                            ReflectionHookError::CommandCanOnlyBeUsedInsideEnum,
                            token.linear_loc});
                        return;
                    }
                    State& state = this->stack.back();
                    if (state.keep_reflected_enum_token.is_valid()) {
                        this->parser->pp->error_handler(new ReflectionHookError{
                            ReflectionHookError::DuplicateCommand, token.linear_loc,
                            state.keep_reflected_enum_token.linear_loc});
                        return;
                    }
                    state.keep_reflected_enum_token = token;
                } else {
                    this->parser->pp->error_handler(new ReflectionHookError{
                        ReflectionHookError::UnrecognizedCommand, token.linear_loc});
                    return;
                }
            }
        }
    }

    virtual bool handle_error(Owned<cpp::BaseError>&& err) override {
        this->any_error = true;
        OutStream out = Console.error();
        err->write_message(out, this->parser->pp->visited_files);
        return true;
    }
};

Tuple<SingleFileReflectionInfo, bool> extract_reflection(ReflectionInfoAggregator* agg,
                                                         StringView rel_path) {
    SingleFileReflectionInfo sfri;
    ReflectionHooks visor;
    visor.file_path = rel_path;
    visor.agg = agg;
    visor.sfri = &sfri;

    cpp::PPVisitedFiles visited_files;
    parse_plywood_src_file(Path.join(Workspace.path, rel_path), &visited_files, &visor);

    return {std::move(sfri), !visor.any_error};
}

//                   ▄▄
//   ▄▄▄▄  ▄▄▄▄   ▄▄▄██  ▄▄▄▄   ▄▄▄▄▄  ▄▄▄▄  ▄▄▄▄▄
//  ██    ██  ██ ██  ██ ██▄▄██ ██  ██ ██▄▄██ ██  ██
//  ▀█▄▄▄ ▀█▄▄█▀ ▀█▄▄██ ▀█▄▄▄  ▀█▄▄██ ▀█▄▄▄  ██  ██
//                              ▄▄▄█▀

struct CodeGenerator {
    virtual ~CodeGenerator() {
    }
    virtual void write(OutStream& out) = 0;
};

String get_switch_inl(SwitchInfo* switch_) {
    MemOutStream mout;
    mout << "enum class ID : u16 {\n";
    for (StringView state : switch_->states) {
        mout.format("    {},\n", state);
    }
    mout << "    Count,\n";
    mout << "};\n";
    mout << "union Storage_ {\n";
    for (StringView state : switch_->states) {
        mout.format("    {} {}{};\n", state, state.left(1).lower_asc(),
                    state.sub_str(1));
    }
    mout << "    Storage_() {}\n";
    mout << "    ~Storage_() {}\n";
    mout << "};\n";
    StringView class_name = switch_->name.split_byte(':').back(); // FIXME: more elegant
    // FIXME: Log an error if there are no states
    mout.format("SWITCH_FOOTER({}, {})\n", class_name, switch_->states[0]);
    for (StringView state : switch_->states) {
        mout.format("SWITCH_ACCESSOR({}, {}{})\n", state, state.left(1).lower_asc(),
                    state.sub_str(1));
    }
    if (switch_->is_reflected) {
        mout << "PLY_SWITCH_REFLECT()\n";
    }
    return mout.move_to_string();
}

void write_switch_inl(SwitchInfo* switch_, const TextFormat& tff) {
    String abs_inl_path = Path.join(Workspace.path, switch_->inline_inl_path);
    FSResult result = FileSystem.make_dirs_and_save_text_if_different(
        abs_inl_path, get_switch_inl(switch_), tff);
    OutStream std_out = Console.out();
    if (result == FSResult::OK) {
        std_out.format("Wrote {}\n", abs_inl_path);
    } else if (result != FSResult::Unchanged) {
        std_out.format("Error writing {}\n", abs_inl_path);
    }
}

String perform_substs(StringView abs_path, ArrayView<Subst> substs) {
    String src = FileSystem.load_text_autodetect(abs_path);
    if (FileSystem.last_result() != FSResult::OK)
        return {};

    MemOutStream mout;
    u32 prev_end_pos = 0;
    for (const Subst& subst : substs) {
        PLY_ASSERT(subst.start >= prev_end_pos);
        u32 end_pos = subst.start + subst.num_bytes;
        PLY_ASSERT(end_pos < src.num_bytes);
        mout << StringView{src.bytes + prev_end_pos, subst.start - prev_end_pos};
        mout << subst.replacement;
        prev_end_pos = end_pos;
    }
    mout << StringView{src.bytes + prev_end_pos, src.num_bytes - prev_end_pos};
    return mout.move_to_string();
}

void perform_substs_and_save(StringView abs_path, ArrayView<Subst> substs,
                             const TextFormat& tff) {
    // FIXME: Don't reload the file here!!!!!!!!!!
    // It may have changed, making the Substs invalid!!!!!!!!
    String src_with_subst = perform_substs(abs_path, substs);
    if (FileSystem.last_result() == FSResult::OK) {
        FSResult result = FileSystem.make_dirs_and_save_text_if_different(
            abs_path, src_with_subst, tff);
        OutStream std_out = Console.out();
        if (result == FSResult::OK) {
            std_out.format("Wrote {}\n", abs_path);
        } else if (result != FSResult::Unchanged) {
            std_out.format("Error writing {}\n", abs_path);
        }
    }
}

void generate_all_cpp_inls(ReflectionInfoAggregator* agg, const TextFormat& tff) {
    struct CodeGenerator {
        virtual ~CodeGenerator() {
        }
        virtual void write(OutStream& out) = 0;
    };

    Map<String, Array<Owned<CodeGenerator>>> file_to_generator_list;

    for (ReflectedClass* clazz : agg->classes) {
        struct StructGenerator : CodeGenerator {
            ReflectedClass* clazz;
            virtual void write(OutStream& out) override {
                out.format("PLY_STRUCT_BEGIN({})\n", this->clazz->name);
                for (StringView member : this->clazz->members) {
                    out.format("PLY_STRUCT_MEMBER({})\n", member);
                }
                out << "PLY_STRUCT_END()\n\n";
            }
            StructGenerator(ReflectedClass* clazz) : clazz{clazz} {
            }
        };
        file_to_generator_list.insert_or_find(clazz->cpp_inl_path)
            ->append(new StructGenerator{clazz});
    }

    for (ReflectedEnum* enum_ : agg->enums) {
        struct EnumGenerator : CodeGenerator {
            ReflectedEnum* enum_;
            virtual void write(OutStream& out) override {
                out.format("PLY_ENUM_BEGIN({}, {})\n", this->enum_->namespace_prefix,
                           this->enum_->enum_name);
                for (u32 i = 0; i < this->enum_->enumerators.num_items(); i++) {
                    StringView enumerator = this->enum_->enumerators[i];
                    if ((i != this->enum_->enumerators.num_items() - 1) ||
                        (enumerator != "Count")) {
                        out.format("PLY_ENUM_IDENTIFIER({})\n", enumerator);
                    }
                }
                out.format("PLY_ENUM_END()\n\n");
            }
            EnumGenerator(ReflectedEnum* enum_) : enum_{enum_} {
            }
        };
        file_to_generator_list.insert_or_find(enum_->cpp_inl_path)
            ->append(new EnumGenerator{enum_});
    }

    for (SwitchInfo* switch_ : agg->switches) {
        struct SwitchGenerator : CodeGenerator {
            SwitchInfo* switch_;
            virtual void write(OutStream& out) override {
                out.format("SWITCH_TABLE_BEGIN({})\n", this->switch_->name);
                for (StringView state : this->switch_->states) {
                    out.format("SWITCH_TABLE_STATE({}, {})\n", this->switch_->name,
                               state);
                }
                out.format("SWITCH_TABLE_END({})\n\n", this->switch_->name);

                if (this->switch_->is_reflected) {
                    out.format("PLY_SWITCH_BEGIN({})\n", this->switch_->name);
                    for (StringView state : this->switch_->states) {
                        out.format("PLY_SWITCH_MEMBER({})\n", state);
                    }
                    out.format("PLY_SWITCH_END()\n\n");
                }
            }
            SwitchGenerator(SwitchInfo* switch_) : switch_{switch_} {
            }
        };
        file_to_generator_list.insert_or_find(switch_->cpp_inl_path)
            ->append(new SwitchGenerator{switch_});
    }

    for (const auto& item : file_to_generator_list) {
        PLY_ASSERT(item.key.ends_with(".inl"));
        String abs_path = Path.join(Workspace.path, item.key);

        MemOutStream mout;
        for (CodeGenerator* generator : item.value) {
            generator->write(mout);
        }
        FSResult result = FileSystem.make_dirs_and_save_text_if_different(
            abs_path, mout.move_to_string(), tff);
        OutStream std_out = Console.out();
        if (result == FSResult::OK) {
            std_out.format("Wrote {}\n", abs_path);
        } else if (result != FSResult::Unchanged) {
            std_out.format("Error writing {}\n", abs_path);
        }
    }
}

void do_codegen() {
    ReflectionInfoAggregator agg;

    u32 file_num = 0;
    for (const FileInfo& entry : FileSystem.list_dir(Workspace.path, 0)) {
        if (!entry.is_dir)
            continue;
        if (entry.name.starts_with("."))
            continue;
        if (entry.name == "data")
            continue;

        for (WalkTriple& triple :
             FileSystem.walk(Path.join(Workspace.path, entry.name))) {
            // Sort child directories and filenames so that files are visited in a
            // deterministic order:
            sort(triple.dir_names);
            sort(triple.files,
                 [](const FileInfo& a, const FileInfo& b) { return a.name < b.name; });

            if (find(triple.files, [](const auto& file_info) {
                    return file_info.name == "nocodegen";
                }) >= 0) {
                triple.dir_names.clear();
                continue;
            }

            for (const FileInfo& file : triple.files) {
                if (file.name.ends_with(".cpp") || file.name.ends_with(".h")) {
                    // FIXME: Eliminate exclusions
                    for (StringView exclude : {
                             "Sort.h",
                             "Func.h",
                             "DirectoryWatcher_Mac.h",
                             "DirectoryWatcher_Win32.h",
                             "Heap.cpp",
                             "Pool.h",
                         }) {
                        if (file.name == exclude)
                            goto skip_it;
                    }
                    {
                        file_num++;

                        Tuple<SingleFileReflectionInfo, bool> sfri = extract_reflection(
                            &agg, Path.join(triple.dir_path, file.name));
                        if (sfri.second) {
                            for (SwitchInfo* switch_ : sfri.first.switches) {
                                write_switch_inl(switch_,
                                                 Workspace.get_source_text_format());
                            }
                            perform_substs_and_save(
                                Path.join(triple.dir_path, file.name),
                                sfri.first.substs_in_parsed_file,
                                Workspace.get_source_text_format());
                        }
                    }
                skip_it:;
                }
            }
            for (StringView exclude : {"Shell_iOS", "opengl-support"}) {
                s32 i = find(triple.dir_names, exclude);
                if (i >= 0) {
                    triple.dir_names.erase(i);
                }
            }
        }
    }

    generate_all_cpp_inls(&agg, Workspace.get_source_text_format());
}

#include "codegen/codegen.inl" //%%

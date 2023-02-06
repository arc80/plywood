/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-biscuit/Parser.h>
#include <ply-biscuit/Interpreter.h>
#include <ply-test/TestSuite.h>
#include <ply-reflect/methods/BoundMethod.h>

using namespace ply;
using namespace ply::biscuit;

struct ScriptOutStream {
    PLY_REFLECT()
    // ply reflect off

    OutStream* outs;
};

FnResult do_print(const FnParams& params) {
    ScriptOutStream* sos = params.self.cast<ScriptOutStream>();
    if (params.args.num_items != 1) {
        params.base->error("'print' expects exactly one argument");
        return Fn_Error;
    }

    const AnyObject& arg = params.args[0];
    params.base->return_value = {};
    if (arg.is<u32>()) {
        *sos->outs << *arg.cast<u32>() << '\n';
    } else if (arg.is<bool>()) {
        *sos->outs << *arg.cast<bool>() << '\n';
    } else if (arg.is<String>()) {
        *sos->outs << *arg.cast<String>() << '\n';
    } else {
        params.base->error(
            String::format("'{}' does not support printing", arg.type->get_name()));
        return Fn_Error;
    }
    return Fn_OK;
}

// FIXME: Move this to the biscuit library:
struct BuiltInStorage {
    BoundMethod bound_print;
    bool true_ = true;
    bool false_ = false;

    BuiltInStorage(ScriptOutStream* sos) {
        this->bound_print = {AnyObject::bind(sos), AnyObject::bind(do_print)};
    }
    void add_built_ins(Map<Label, AnyObject>& ns) {
        ns.assign(g_labelStorage.insert("print"), AnyObject::bind(&this->bound_print));
        ns.assign(g_labelStorage.insert("true"), AnyObject::bind(&this->true_));
        ns.assign(g_labelStorage.insert("false"), AnyObject::bind(&this->false_));
    }
};

String parse_and_try_call(StringView src, StringView func_name,
                          ArrayView<const AnyObject> args) {
    // Create tokenizer and parser.
    Tokenizer tkr;
    tkr.set_source_input({}, src);
    Parser parser;
    parser.tkr = &tkr;
    MemOutStream error_out;
    parser.error = [&error_out](StringView message) { error_out << message; };
    parser.filter.allow_functions = true;
    parser.filter.allow_instructions = false;
    Map<Label, Owned<Statement>> fn_map;
    parser.function_handler = [&parser, &fn_map](Owned<biscuit::Statement>&& stmt,
                                                 const ExpandedToken& name_token) {
        if (!parse_parameter_list(&parser, stmt->function_definition().get()))
            return;

        // Parse function body.
        biscuit::Parser::Filter filter;
        filter.keyword_handler = [](const KeywordParams&) {
            return KeywordResult::Illegal;
        };
        filter.allow_instructions = true;
        PLY_SET_IN_SCOPE(parser.filter, filter);
        stmt->function_definition()->body =
            parse_statement_block(&parser, {"function", "parameter list"});

        fn_map.assign(name_token.label, std::move(stmt));
    };

    // Parse the script.
    biscuit::StatementBlockProperties props{"file"};
    Owned<biscuit::StatementBlock> file =
        parse_statement_block_inner(&parser, props, true);
    if (parser.error_count > 0)
        return error_out.move_to_string();

    // Invoke function if it exists
    if (Owned<Statement>* stmt = fn_map.find(g_labelStorage.find(func_name))) {
        const Statement::FunctionDefinition* fn_def =
            (*stmt)->function_definition().get();
        MemOutStream outs;
        ScriptOutStream sos{&outs};
        BuiltInStorage bis{&sos};

        // Create interpreter
        Interpreter interp;
        interp.base.error = [&outs, &interp](StringView message) {
            log_error_with_stack(outs, &interp, message);
        };
        Map<Label, AnyObject> bi_map;
        bis.add_built_ins(bi_map);
        interp.resolve_name = [&bi_map, &fn_map](Label identifier) -> AnyObject {
            if (AnyObject* built_in = bi_map.find(identifier))
                return *built_in;
            if (Owned<Statement>* found_stmt = fn_map.find(identifier))
                return AnyObject::bind((*found_stmt)->function_definition().get());
            return {};
        };

        // Invoke function
        Interpreter::StackFrame frame;
        frame.interp = &interp;
        frame.desc = [fn_def]() -> HybridString {
            return String::format("function '{}'", g_labelStorage.view(fn_def->name));
        };
        frame.tkr = &tkr;
        PLY_ASSERT(fn_def->parameter_names.num_items() == args.num_items);
        for (u32 i = 0; i < args.num_items; i++) {
            frame.local_variable_table.assign(fn_def->parameter_names[i], args[i]);
        }
        exec_function(&frame, fn_def->body);

        return outs.move_to_string();
    }

    return {};
}

#define PLY_TEST_CASE_PREFIX Biscuit_

// Runtime reflection lets us access struct members from script.
struct TestStruct {
    PLY_REFLECT()
    String name;
    u32 value = 0;
    // ply reflect off
};

PLY_TEST_CASE("Manipulate a C++ object from script") {
    // This C++ object will be passed into the script.
    TestStruct obj;
    obj.name = "banana";
    obj.value = 12;

    // Script source code.
    StringView script = R"(
fn test(obj) {
    print(obj.name)
    print(obj.value)
    obj.name = "orange"
    obj.value = obj.value + 1
}
)";

    // Parse the script and call function "test" with obj as its argument.
    String result = parse_and_try_call(script, "test", {AnyObject::bind(&obj)});

    // Check result.
    PLY_TEST_CHECK(result == "banana\n12\n");
    PLY_TEST_CHECK(obj.name == "orange");
    PLY_TEST_CHECK(obj.value == 13);
}

void run_test_suite() {
    struct SectionReader {
        ViewInStream ins;
        SectionReader(StringView view) : ins{view} {
        }
        StringView read_section() {
            StringView line = ins.read_view<fmt::Line>();
            StringView first = line;
            StringView last = line;
            bool got_result = false;
            while (line && !line.starts_with("----")) {
                line = ins.read_view<fmt::Line>();
                if (!got_result) {
                    last = line;
                }
                if (line.starts_with("result:")) {
                    got_result = true;
                }
            }
            StringView result = StringView::from_range(first.bytes, last.bytes);
            if (result.ends_with("\n\n")) {
                result = result.shortened_by(1);
            }
            return result;
        }
    };

    // Open input file
    String test_suite_path = Path.join(get_workspace_path(),
                                       "base/src/apps/BiscuitTest/AllBiscuitTests.txt");
    String all_tests = FileSystem.load_text_autodetect(test_suite_path);
    SectionReader sr{all_tests};

    // Iterate over test cases.
    MemOutStream outs;
    for (;;) {
        StringView src = sr.read_section();
        if (!src) {
            if (sr.ins.at_eof())
                break;
            continue;
        }

        // Run this test case
        String output = parse_and_try_call(src, "test", {});

        // Append to result
        outs << StringView{"-"} * 60 << '\n';
        outs << src;
        if (!src.ends_with("\n\n")) {
            outs << "\n";
        }
        if (output) {
            outs << "result:\n";
            outs << output;
            if (!output.is_empty() && !output.ends_with('\n')) {
                outs << "\n";
            }
            outs << "\n";
        }
    }

    // Rewrite BiscuitTests.txt
    FileSystem.make_dirs_and_save_text_if_different(test_suite_path,
                                                    outs.move_to_string());
}

int main() {
    run_test_suite();
    return ply::test::run() ? 0 : 1;
}

#include "codegen/Main.inl" //%%

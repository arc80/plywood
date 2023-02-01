/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
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

FnResult doPrint(const FnParams& params) {
    ScriptOutStream* sos = params.self.cast<ScriptOutStream>();
    if (params.args.numItems != 1) {
        params.base->error("'print' expects exactly one argument");
        return Fn_Error;
    }

    const AnyObject& arg = params.args[0];
    params.base->returnValue = {};
    if (arg.is<u32>()) {
        *sos->outs << *arg.cast<u32>() << '\n';
    } else if (arg.is<bool>()) {
        *sos->outs << *arg.cast<bool>() << '\n';
    } else if (arg.is<String>()) {
        *sos->outs << *arg.cast<String>() << '\n';
    } else {
        params.base->error(
            String::format("'{}' does not support printing", arg.type->getName()));
        return Fn_Error;
    }
    return Fn_OK;
}

// FIXME: Move this to the biscuit library:
struct BuiltInStorage {
    BoundMethod boundPrint;
    bool true_ = true;
    bool false_ = false;

    BuiltInStorage(ScriptOutStream* sos) {
        this->boundPrint = {AnyObject::bind(sos), AnyObject::bind(doPrint)};
    }
    void addBuiltIns(LabelMap<AnyObject>& ns) {
        *ns.insert(g_labelStorage.insert("print")) = AnyObject::bind(&this->boundPrint);
        *ns.insert(g_labelStorage.insert("true")) = AnyObject::bind(&this->true_);
        *ns.insert(g_labelStorage.insert("false")) = AnyObject::bind(&this->false_);
    }
};

String parseAndTryCall(StringView src, StringView funcName,
                       ArrayView<const AnyObject> args) {
    // Create tokenizer and parser.
    Tokenizer tkr;
    tkr.setSourceInput({}, src);
    Parser parser;
    parser.tkr = &tkr;
    MemOutStream errorOut;
    parser.error = [&errorOut](StringView message) { errorOut << message; };
    parser.filter.allowFunctions = true;
    parser.filter.allowInstructions = false;
    LabelMap<Owned<Statement>> fnMap;
    parser.functionHandler = [&parser, &fnMap](Owned<biscuit::Statement>&& stmt,
                                               const ExpandedToken& nameToken) {
        if (!parseParameterList(&parser, stmt->functionDefinition().get()))
            return;

        // Parse function body.
        biscuit::Parser::Filter filter;
        filter.keywordHandler = [](const KeywordParams&) {
            return KeywordResult::Illegal;
        };
        filter.allowInstructions = true;
        PLY_SET_IN_SCOPE(parser.filter, filter);
        stmt->functionDefinition()->body =
            parseStatementBlock(&parser, {"function", "parameter list"});

        *fnMap.insert(nameToken.label) = std::move(stmt);
    };

    // Parse the script.
    biscuit::StatementBlockProperties props{"file"};
    Owned<biscuit::StatementBlock> file =
        parseStatementBlockInner(&parser, props, true);
    if (parser.errorCount > 0)
        return errorOut.moveToString();

    // Invoke function if it exists
    if (Owned<Statement>* stmt = fnMap.find(g_labelStorage.find(funcName))) {
        const Statement::FunctionDefinition* fnDef =
            (*stmt)->functionDefinition().get();
        MemOutStream outs;
        ScriptOutStream sos{&outs};
        BuiltInStorage bis{&sos};

        // Create interpreter
        Interpreter interp;
        interp.base.error = [&outs, &interp](StringView message) {
            logErrorWithStack(outs, &interp, message);
        };
        LabelMap<AnyObject> biMap;
        bis.addBuiltIns(biMap);
        interp.resolveName = [&biMap, &fnMap](Label identifier) -> AnyObject {
            if (AnyObject* builtIn = biMap.find(identifier))
                return *builtIn;
            if (Owned<Statement>* foundStmt = fnMap.find(identifier))
                return AnyObject::bind((*foundStmt)->functionDefinition().get());
            return {};
        };

        // Invoke function
        Interpreter::StackFrame frame;
        frame.interp = &interp;
        frame.desc = [fnDef]() -> HybridString {
            return String::format("function '{}'", g_labelStorage.view(fnDef->name));
        };
        frame.tkr = &tkr;
        PLY_ASSERT(fnDef->parameterNames.numItems() == args.numItems);
        for (u32 i = 0; i < args.numItems; i++) {
            *frame.localVariableTable.insert(fnDef->parameterNames[i]) = args[i];
        }
        execFunction(&frame, fnDef->body);

        return outs.moveToString();
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
    String result = parseAndTryCall(script, "test", {AnyObject::bind(&obj)});

    // Check result.
    PLY_TEST_CHECK(result == "banana\n12\n");
    PLY_TEST_CHECK(obj.name == "orange");
    PLY_TEST_CHECK(obj.value == 13);
}

void runTestSuite() {
    struct SectionReader {
        ViewInStream ins;
        SectionReader(StringView view) : ins{view} {
        }
        StringView readSection() {
            StringView line = ins.readView<fmt::Line>();
            StringView first = line;
            StringView last = line;
            bool gotResult = false;
            while (line && !line.startsWith("----")) {
                line = ins.readView<fmt::Line>();
                if (!gotResult) {
                    last = line;
                }
                if (line.startsWith("result:")) {
                    gotResult = true;
                }
            }
            StringView result = StringView::fromRange(first.bytes, last.bytes);
            if (result.endsWith("\n\n")) {
                result = result.shortenedBy(1);
            }
            return result;
        }
    };

    // Open input file
    String testSuitePath = Path.join(get_workspace_path(),
                                     "base/src/apps/BiscuitTest/AllBiscuitTests.txt");
    String allTests = FileSystem.loadTextAutodetect(testSuitePath);
    SectionReader sr{allTests};

    // Iterate over test cases.
    MemOutStream outs;
    for (;;) {
        StringView src = sr.readSection();
        if (!src) {
            if (sr.ins.at_eof())
                break;
            continue;
        }

        // Run this test case
        String output = parseAndTryCall(src, "test", {});

        // Append to result
        outs << StringView{"-"} * 60 << '\n';
        outs << src;
        if (!src.endsWith("\n\n")) {
            outs << "\n";
        }
        if (output) {
            outs << "result:\n";
            outs << output;
            if (!output.isEmpty() && !output.endsWith('\n')) {
                outs << "\n";
            }
            outs << "\n";
        }
    }

    // Rewrite BiscuitTests.txt
    FileSystem.makeDirsAndSaveTextIfDifferent(testSuitePath, outs.moveToString());
}

int main() {
    runTestSuite();
    return ply::test::run() ? 0 : 1;
}

#include "codegen/Main.inl" //%%

/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-crowbar/Parser.h>
#include <ply-crowbar/Interpreter.h>
#include <ply-runtime/algorithm/Find.h>
#include <ply-test/TestSuite.h>
#include <ply-reflect/methods/BoundMethod.h>

using namespace ply;
using namespace ply::crowbar;

struct ScriptOutStream {
    PLY_REFLECT()
    // ply reflect off

    OutStream* outs;
};

MethodResult doPrint(BaseInterpreter* interp, const AnyObject& callee,
                     ArrayView<const AnyObject> args) {
    ScriptOutStream* sos = callee.cast<ScriptOutStream>();

    if (args.numItems != 1) {
        interp->error("'print' expects exactly one argument");
        return MethodResult::Error;
    }

    const AnyObject& arg = args[0];
    interp->returnValue = {};
    if (arg.is<u32>()) {
        *sos->outs << *arg.cast<u32>() << '\n';
    } else if (arg.is<bool>()) {
        *sos->outs << *arg.cast<bool>() << '\n';
    } else if (arg.is<String>()) {
        *sos->outs << *arg.cast<String>() << '\n';
    } else {
        interp->error(String::format("'{}' does not support printing", arg.type->getName()));
        return MethodResult::Error;
    }
    return MethodResult::OK;
}

// FIXME: Move this to the crowbar module:
void addBuiltIns(LabelMap<AnyObject>& ns, ScriptOutStream* sos) {
    static BoundMethod boundPrint = {AnyObject::bind(sos), AnyObject::bind(doPrint)};
    *ns.insert(g_labelStorage.insert("print")) = AnyObject::bind(&boundPrint);

    static bool true_ = true;
    static bool false_ = false;
    *ns.insert(g_labelStorage.insert("true")) = AnyObject::bind(&true_);
    *ns.insert(g_labelStorage.insert("false")) = AnyObject::bind(&false_);
}

String parseAndTryCall(StringView src, StringView funcName, ArrayView<const AnyObject> args) {
    // Create tokenizer and parser.
    Tokenizer tkr;
    tkr.setSourceInput({}, src);
    Parser parser;
    parser.tkr = &tkr;
    MemOutStream errorOut;
    parser.error = [&errorOut](StringView message) { errorOut << message; };

    // Parse the script.
    Owned<StatementBlock> file = parser.parseFile();
    if (parser.errorCount > 0)
        return errorOut.moveToString();

    // Put functions in a namespace
    LabelMap<const Statement::FunctionDefinition*> fnMap;
    for (const Statement* stmt : file->statements) {
        const Statement::FunctionDefinition* fnDef = stmt->functionDefinition().get();
        *fnMap.insert(fnDef->name) = fnDef;
    }

    // Invoke function if it exists
    const Statement::FunctionDefinition** fnObj = fnMap.find(g_labelStorage.find(funcName));
    if (fnObj) {
        MemOutStream outs;
        ScriptOutStream sos{&outs};

        // Create interpreter
        Interpreter interp;
        interp.base.error = [](StringView message) { StdErr::text() << message; };
        addBuiltIns(interp.builtIns, &sos);
        interp.hooks.resolveName = [&fnMap](Label identifier) -> AnyObject {
            const Statement::FunctionDefinition** fnObj = fnMap.find(identifier);
            return fnObj ? AnyObject::bind(*fnObj) : AnyObject{};
        };

        // Invoke function
        Interpreter::StackFrame frame;
        frame.interp = &interp;
        frame.desc = [fnDef = *fnObj]() -> HybridString {
            return String::format("function '{}'", g_labelStorage.view(fnDef->name));
        };
        frame.tkr = &tkr;
        PLY_ASSERT((*fnObj)->parameterNames.numItems() == args.numItems);
        for (u32 i = 0; i < args.numItems; i++) {
            *frame.localVariableTable.insert((*fnObj)->parameterNames[i]) = args[i];
        }
        execFunction(&frame, (*fnObj)->body);

        return outs.moveToString();
    }

    return {};
}

#define PLY_TEST_CASE_PREFIX Crowbar_

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
    String testSuitePath =
        NativePath::join(PLY_WORKSPACE_FOLDER, "base/src/apps/CrowbarTest/AllCrowbarTests.txt");
    Tuple<String, TextFormat> allTests = FileSystem::native()->loadTextAutodetect(testSuitePath);
    SectionReader sr{allTests.first};

    // Iterate over test cases.
    MemOutStream outs;
    for (;;) {
        StringView src = sr.readSection();
        if (!src) {
            if (sr.ins.atEOF())
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

    // Rewrite CrowbarTests.txt
    FileSystem::native()->makeDirsAndSaveTextIfDifferent(testSuitePath, outs.moveToString(),
                                                         allTests.second);
}

int main() {
    runTestSuite();
    return ply::test::run() ? 0 : 1;
}

#include "codegen/Main.inl" //%%

/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-crowbar/Parser.h>
#include <ply-crowbar/Interpreter.h>
#include <ply-runtime/algorithm/Find.h>
#include <ply-test/TestSuite.h>

using namespace ply;
using namespace ply::crowbar;

MethodResult doPrint(BaseInterpreter* interp, const AnyObject& arg) {
    interp->returnValue = {};
    if (arg.is<u32>()) {
        *interp->outs << *arg.cast<u32>() << '\n';
    } else if (arg.is<bool>()) {
        *interp->outs << *arg.cast<bool>() << '\n';
    } else if (arg.is<String>()) {
        *interp->outs << *arg.cast<String>() << '\n';
    } else {
        interp->error(interp,
                      String::format("'{}' does not support printing", arg.type->getName()));
        return MethodResult::Error;
    }
    return MethodResult::OK;
}

// FIXME: Move this to the crowbar module:
void addBuiltIns(InternedStrings& internedStrings, HashMap<VariableMapTraits>& ns) {
    getTypeDescriptor(doPrint);
    ns.insertOrFind(internedStrings.findOrInsertKey("print"))->obj = AnyObject::bind(doPrint);

    static bool true_ = true;
    static bool false_ = false;
    ns.insertOrFind(internedStrings.findOrInsertKey("true"))->obj = AnyObject::bind(&true_);
    ns.insertOrFind(internedStrings.findOrInsertKey("false"))->obj = AnyObject::bind(&false_);
}

String callScriptFunction(StringView src, StringView funcName, ArrayView<const AnyObject> args) {
    // Create tokenizer and parser.
    Tokenizer tkr;
    InternedStrings internedStrings;
    tkr.internedStrings = &internedStrings;
    tkr.setSourceInput(src);
    Parser parser;
    parser.tkr = &tkr;

    // Implement hooks.
    struct Hooks : Parser::Hooks {
        MemOutStream errorOut;
        u32 errorCount = 0;
        virtual void onError(StringView errorMsg) override {
            this->errorOut << errorMsg;
            this->errorCount++;
        }
    };
    Hooks hooks;
    parser.hooks = &hooks;

    // Parse the script.
    Owned<File> file = parser.parseFile();
    if (hooks.errorCount > 0)
        return hooks.errorOut.moveToString();

    // Create built-in namespace
    HashMap<VariableMapTraits> builtIns;
    addBuiltIns(internedStrings, builtIns);

    // Put functions in a namespace
    Sequence<AnyObject> fnObjs;
    HashMap<VariableMapTraits> ns;
    for (const FunctionDefinition* fnDef : file->functions) {
        const AnyObject& fnObj = fnObjs.append(AnyObject::bind(fnDef));
        ns.insertOrFind(fnDef->name)->obj = fnObj;
    }

    // Invoke function if it exists
    auto testFuncCursor = ns.find(internedStrings.findKey(funcName));
    if (testFuncCursor.wasFound()) {
        const AnyObject& testObj = testFuncCursor->obj;
        MemOutStream outs;

        // Create interpreter
        Interpreter interp;
        interp.outs = &outs;
        interp.internedStrings = &internedStrings;
        interp.outerNameSpaces.append(&builtIns);
        interp.outerNameSpaces.append(&ns);
        interp.error = [](BaseInterpreter* base, StringView message) {
            Interpreter* interp = static_cast<Interpreter*>(base);
            interp->outs->format("error: {}\n", message);
            bool first = true;
            for (Interpreter::StackFrame* frame = interp->currentFrame; frame;
                 frame = frame->prevFrame) {
                ExpandedToken expToken = interp->tkr->expandToken(frame->tokenIdx);
                interp->outs->format(
                    "{} {} '{}'\n",
                    interp->tkr->fileLocationMap.formatFileLocation(expToken.fileOffset),
                    first ? "in function" : "called from",
                    interp->internedStrings->view(frame->functionDef->name));
                first = false;
            }
        };
        interp.tkr = &tkr;

        // Invoke function
        Interpreter::StackFrame frame;
        frame.interp = &interp;
        const FunctionDefinition* fnDef = testObj.cast<FunctionDefinition>();
        frame.functionDef = fnDef;
        PLY_ASSERT(fnDef->parameterNames.numItems() == args.numItems);
        for (u32 i = 0; i < args.numItems; i++) {
            frame.localVariableTable.insertOrFind(fnDef->parameterNames[i])->obj = args[i];
        }
        execFunction(&frame, fnDef->body);

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
    String result = callScriptFunction(script, "test", {AnyObject::bind(&obj)});

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
    String testSuitePath = NativePath::join(
        PLY_WORKSPACE_FOLDER, "repos/plywood/src/apps/CrowbarTest/AllCrowbarTests.txt");
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
        String output = callScriptFunction(src, "test", {});

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
    // return ply::test::run() ? 0 : 1;
    return 0;
}

#include "codegen/Main.inl" //%%

/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-crowbar/Parser.h>
#include <ply-crowbar/Interpreter.h>
#include <ply-runtime/algorithm/Find.h>

using namespace ply;
using namespace ply::crowbar;

OutStream* doPrintTarget = nullptr;

AnyObject doPrint(ObjectStack* stack, const AnyObject& arg) {
    if (arg.is<u32>()) {
        *doPrintTarget << *arg.cast<u32>() << '\n';
    } else if (arg.is<bool>()) {
        *doPrintTarget << *arg.cast<bool>() << '\n';
    } else if (arg.is<String>()) {
        *doPrintTarget << *arg.cast<String>() << '\n';
    }
    return {};
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

String runTestCase(StringView src) {
    // Create tokenizer
    Tokenizer tkr;
    InternedStrings internedStrings;
    tkr.internedStrings = &internedStrings;
    tkr.setSourceInput(src);

    // Create parser
    Parser parser;
    parser.tkr = &tkr;

    // Implement hooks
    MemOutStream errorOut;
    struct Hooks : Parser::Hooks {
        MemOutStream& errorOut;
        u32 errorCount = 0;
        virtual void onError(StringView errorMsg) override {
            this->errorOut << errorMsg;
            this->errorCount++;
        }
        Hooks(MemOutStream& errorOut) : errorOut{errorOut} {
        }
    };
    Hooks hooks{errorOut};
    parser.hooks = &hooks;

    // Parse the expression
    Owned<File> file = parser.parseFile();

    String output;
    if (hooks.errorCount > 0) {
        output = errorOut.moveToString();
    } else {
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

        // Invoke "test" function if it exists
        auto testFuncCursor = ns.find(internedStrings.findOrInsertKey("test"));
        if (testFuncCursor.wasFound()) {
            MemOutStream outs;
            doPrintTarget = &outs;
            ns.insertOrFind(internedStrings.findOrInsertKey("print"))->obj =
                AnyObject::bind(doPrint);

            // Create interpreter
            Interpreter interp;
            interp.internedStrings = &internedStrings;
            interp.outerNameSpaces.append(&builtIns);
            interp.outerNameSpaces.append(&ns);

            // Invoke function
            const AnyObject& testObj = testFuncCursor->obj;
            interp.startStackFrame(testObj.cast<FunctionDefinition>()->body);
            interp.finish();

            output = outs.moveToString();
        }
    }
    return output;
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
        String output = runTestCase(src);

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
    return 0;
}

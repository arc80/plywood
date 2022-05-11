/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-crowbar/Core.h>
#include <ply-reflect/methods/BaseInterpreter.h>
#include <ply-crowbar/ParseTree.h>

namespace ply {
namespace crowbar {

struct Tokenizer;

struct VariableMapTraits {
    struct Item {
        u32 key = 0;
        AnyObject obj;
        Item(u32 key) : key{key} {
        }
    };
    using Key = u32;
    static PLY_INLINE bool match(const Item& item, Key key) {
        return item.key == key;
    }
};

struct Interpreter : BaseInterpreter {
    struct Hooks {
        virtual ~Hooks() {}
        virtual void enterCustomBlock(const Statement::CustomBlock* customBlock) {}
        virtual void exitCustomBlock(const Statement::CustomBlock* customBlock) {}
        virtual void onEvaluate(const AnyObject& evaluationTraits) {}
    };

    struct StackFrame {
        Interpreter* interp = nullptr;
        const Statement::FunctionDefinition* functionDef = nullptr;
        HashMap<VariableMapTraits> localVariableTable;
        u32 tokenIdx = 0;
        StackFrame* prevFrame = nullptr;
    };

    const InternedStrings* internedStrings = nullptr;
    Array<HashMap<VariableMapTraits>*> outerNameSpaces;
    Hooks* hooks = nullptr;

    // For expanding the location of runtime errors:
    Tokenizer* tkr = nullptr;
    StackFrame *currentFrame = nullptr;
};

MethodResult execFunction(Interpreter::StackFrame* frame, const StatementBlock* block);

} // namespace crowbar
} // namespace ply

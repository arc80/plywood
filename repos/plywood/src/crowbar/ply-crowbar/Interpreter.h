/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-crowbar/Core.h>
#include <ply-reflect/methods/BaseInterpreter.h>
#include <ply-runtime/string/Label.h>
#include <ply-crowbar/ParseTree.h>

namespace ply {
namespace crowbar {

struct Tokenizer;

struct VariableMapTraits {
    using Key = Label;
    struct Item {
        Label identifier;
        AnyObject obj;
        Item(Label identifier) : identifier{identifier} {
        }
    };
    static PLY_INLINE bool match(const Item& item, const Label& identifier) {
        return item.identifier == identifier;
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
        HiddenArgFunctor<HybridString()> desc;
        HashMap<VariableMapTraits> localVariableTable;
        u32 tokenIdx = 0;
        StackFrame* prevFrame = nullptr;
    };

    Array<HashMap<VariableMapTraits>*> outerNameSpaces;
    Hooks* hooks = nullptr;

    // For expanding the location of runtime errors:
    Tokenizer* tkr = nullptr;
    StackFrame *currentFrame = nullptr;

    // Custom block support
    const Statement::CustomBlock* customBlock = nullptr;
};

MethodResult execFunction(Interpreter::StackFrame* frame, const StatementBlock* block);

} // namespace crowbar
} // namespace ply

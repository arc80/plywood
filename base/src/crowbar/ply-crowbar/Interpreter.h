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

struct Interpreter  {
    struct Hooks {
        Functor<AnyObject(Label identifier)> resolveName;
        Functor<void(const Statement::CustomBlock* customBlock, bool enter)> customBlock;
        Functor<void(const AnyObject& evaluationTraits)> onEvaluate;
        Functor<bool(Label label)> assignToLocal;
    };

    struct StackFrame {
        Interpreter* interp = nullptr;
        Functor<HybridString()> desc;
        LabelMap<AnyObject> localVariableTable;
        Tokenizer* tkr = nullptr;
        const Statement::CustomBlock* customBlock = nullptr;
        u32 tokenIdx = 0;
        StackFrame* prevFrame = nullptr;
    };

    BaseInterpreter base;
    Hooks hooks;
    LabelMap<AnyObject> builtIns;
    StackFrame* currentFrame = nullptr;
};

Functor<HybridString()> makeFunctionDesc(const Statement::FunctionDefinition* fnDef);
MethodResult execFunction(Interpreter::StackFrame* frame, const StatementBlock* block);
MethodResult eval(Interpreter::StackFrame* frame, const Expression* expr);

} // namespace crowbar
} // namespace ply

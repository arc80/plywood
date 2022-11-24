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

struct Interpreter {
    struct Hooks {
        Functor<MethodResult(const Statement::CustomBlock* customBlock)> doCustomBlock;
        Functor<bool(const AnyObject& attributes)> onEvaluate = [](const AnyObject&) {
            return true;
        };
        Functor<bool(const AnyObject& attributes, Label label)> assignToLocal;
    };

    struct StackFrame {
        Interpreter* interp = nullptr;
        Functor<HybridString()> desc;
        LabelMap<AnyObject> localVariableTable;
        Tokenizer* tkr = nullptr;
        const Statement::CustomBlock* customBlock = nullptr;
        u32 tokenIdx = 0;
        StackFrame* prevFrame = nullptr;
        Hooks hooks;
    };

    BaseInterpreter base;
    Functor<AnyObject(Label identifier)> resolveName;
    StackFrame* currentFrame = nullptr;
};

void logErrorWithStack(OutStream* outs, const Interpreter* interp, StringView message);
MethodResult execFunction(Interpreter::StackFrame* frame, const StatementBlock* block);
MethodResult execBlock(Interpreter::StackFrame* frame, const StatementBlock* block);
MethodResult eval(Interpreter::StackFrame* frame, const Expression* expr);

} // namespace crowbar
} // namespace ply

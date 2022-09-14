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

struct INamespace {
    virtual ~INamespace() {
    }
    virtual AnyObject find(Label identifier) const = 0;
};

struct MapNamespace : INamespace {
    LabelMap<AnyObject> map;
    virtual AnyObject find(Label identifier) const override;
};

struct Interpreter : BaseInterpreter {
    struct StackFrame {
        Interpreter* interp = nullptr;
        Functor<HybridString()> desc;
        LabelMap<AnyObject> localVariableTable;
        Tokenizer* tkr = nullptr;
        const Statement::CustomBlock* customBlock = nullptr;
        u32 tokenIdx = 0;
        StackFrame* prevFrame = nullptr;
    };

    struct Hooks {
        virtual ~Hooks() {
        }
        virtual void enterCustomBlock(const Statement::CustomBlock* customBlock) {
        }
        virtual void exitCustomBlock(const Statement::CustomBlock* customBlock) {
        }
        virtual void onEvaluate(const AnyObject& evaluationTraits) {
        }
        virtual bool handleLocalAssignment(Label label) {
            return false;
        }
    };

    Array<INamespace*> outerNameSpaces;
    Hooks defaultHooks;
    Hooks* hooks = nullptr;

    // For expanding the location of runtime errors:
    StackFrame* currentFrame = nullptr;

    Interpreter() : hooks{&this->defaultHooks} {
    }
    virtual void error(StringView message) override;
};

Functor<HybridString()> makeFunctionDesc(const Statement::FunctionDefinition* fnDef);
MethodResult execFunction(Interpreter::StackFrame* frame, const StatementBlock* block);
MethodResult eval(Interpreter::StackFrame* frame, const Expression* expr);

} // namespace crowbar
} // namespace ply

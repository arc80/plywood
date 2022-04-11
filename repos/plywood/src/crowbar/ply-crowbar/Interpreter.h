/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-crowbar/Core.h>
#include <ply-reflect/methods/ObjectStack.h>
#include <ply-crowbar/ParseTree.h>

namespace ply {
namespace crowbar {

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

struct Breadcrumb {
    // ply make switch
    struct DoBlock {
        const StatementBlock* block = nullptr;
        u32 stage = 0;
    };
    struct DoEval {
        const Statement::Evaluate* eval = nullptr;
    };
    struct DoAssign {
        const Statement::Assignment* assign = nullptr;
        u32 stage = 0;
        AnyObject left; // blank if assigning to local variable
    };
    struct DoBinaryOp {
        const Expression::BinaryOp* binaryOp = nullptr;
        u32 stage = 0;
        AnyObject left;
    };
    struct DoCall {
        const Expression::Call* call = nullptr;
        u32 stage = 0;
        AnyObject callee;
        Array<AnyObject*> args;
    };
    struct DoReturn {
        const Statement::Return_* return_ = nullptr;
    };
    struct DoIf {
        const Statement::If_* if_ = nullptr;
        u32 stage = 0;
    };
    struct DoWhile {
        const Statement::While_* while_ = nullptr;
        u32 stage = 0;
    };
#include "codegen/switch-ply-crowbar-Breadcrumb.inl" //@@ply
};

struct Interpreter {
    struct FunctionStackFrame {
        Sequence<Breadcrumb> location;
        HashMap<VariableMapTraits> localVariableTable;
        ObjectStack::Boundary endOfPreviousFrameStorage;
        ObjectStack::Boundary localVariableStorageBoundary;
    };

    const InternedStrings* internedStrings = nullptr;
    ObjectStack localVariableStorage;
    Array<FunctionStackFrame> stackFrames;
    AnyObject returnValue;
    Array<HashMap<VariableMapTraits>*> outerNameSpaces;

    Interpreter();
    void startStackFrame(const StatementBlock* block,
                         HashMap<VariableMapTraits>* localVariableTable = nullptr);
    void step();
    void finish();
};

} // namespace crowbar
} // namespace ply
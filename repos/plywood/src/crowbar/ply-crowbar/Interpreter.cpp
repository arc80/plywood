/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-crowbar/Core.h>
#include <ply-crowbar/Interpreter.h>

namespace ply {
namespace crowbar {

enum class StepStatus {
    Completed,
    Continue,
};

StepStatus stepBinaryOp(Interpreter* interp, Breadcrumb::DoBinaryOp* binaryOpCrumb);
StepStatus stepCall(Interpreter* interp, Breadcrumb::DoCall* callCrumb);
StepStatus stepString(Interpreter* interp, Breadcrumb::DoString* stringCrumb);

void destroyStackFrame(Interpreter* interp, bool withReturn = false) {
    Interpreter::FunctionStackFrame& stackFrame = interp->stackFrames.back();
    WeakSequenceRef<AnyObject> deleteTo = interp->localVariableStorage.items.end();
    bool fixupReturnValue = withReturn && (PLY_PTR_OFFSET(interp->returnValue.data,
                                                          interp->returnValue.type->fixedSize) ==
                                           interp->localVariableStorage.items.end().byte());
    if (fixupReturnValue) {
        PLY_ASSERT(interp->localVariableStorage.items.tail() == interp->returnValue);
        --deleteTo;
    }
    interp->localVariableStorage.deleteRange(stackFrame.endOfPreviousFrameStorage, deleteTo);
    if (fixupReturnValue) {
        PLY_ASSERT(interp->localVariableStorage.items.tail().type == interp->returnValue.type);
        interp->returnValue.data = interp->localVariableStorage.items.tail().data;
    }
    interp->stackFrames.pop();
}

PLY_INLINE StepStatus popTailLocation(Interpreter* interp) {
    Interpreter::FunctionStackFrame& stackFrame = interp->stackFrames.back();
    stackFrame.location.popTail();
    if (stackFrame.location.isEmpty()) {
        destroyStackFrame(interp);
        return StepStatus::Completed;
    }
    if (stackFrame.location.tail().id == Breadcrumb::ID::DoBlock) {
        return StepStatus::Completed;
    }
    return StepStatus::Continue;
}

StepStatus startExpression(Interpreter* interp, const Expression* expr) {
    switch (expr->id) {
        case Expression::ID::NameLookup: {
            u32 name = expr->nameLookup()->name;
            Interpreter::FunctionStackFrame& stackFrame = interp->stackFrames.back();
            {
                auto cursor = stackFrame.localVariableTable.find(name);
                if (cursor.wasFound()) {
                    interp->returnValue = cursor->obj;
                    return StepStatus::Continue;
                }
            }

            for (s32 i = interp->outerNameSpaces.numItems() - 1; i >= 0; i--) {
                const HashMap<VariableMapTraits>* ns = interp->outerNameSpaces[i];
                auto cursor = ns->find(name);
                if (cursor.wasFound()) {
                    interp->returnValue = cursor->obj;
                    return StepStatus::Continue;
                }
            }

            PLY_ASSERT(0);
            return StepStatus::Completed;
        }

        case Expression::ID::IntegerLiteral: {
            interp->returnValue = AnyObject::bind(&expr->integerLiteral()->value);
            return StepStatus::Continue;
        }

        case Expression::ID::InterpolatedString: {
            auto stringCrumb = interp->stackFrames.back().location.append().doString().switchTo();
            stringCrumb->string = expr->interpolatedString().get();
            return stepString(interp, stringCrumb.get());
        }

        case Expression::ID::BinaryOp: {
            auto binaryOpCrumb =
                interp->stackFrames.back().location.append().doBinaryOp().switchTo();
            binaryOpCrumb->binaryOp = expr->binaryOp().get();
            return stepBinaryOp(interp, binaryOpCrumb.get());
        }

        case Expression::ID::Call: {
            auto callCrumb = interp->stackFrames.back().location.append().doCall().switchTo();
            callCrumb->call = expr->call().get();
            return stepCall(interp, callCrumb.get());
        }

        default: {
            PLY_ASSERT(0);
            return StepStatus::Completed;
        }
    }
}

StepStatus stepEval(Interpreter* interp, Breadcrumb::DoEval* evalCrumb) {
    Interpreter::FunctionStackFrame& stackFrame = interp->stackFrames.back();
    interp->localVariableStorage.deleteRange(stackFrame.localVariableStorageBoundary,
                                             interp->localVariableStorage.items.end());
    PLY_ASSERT(stackFrame.location.tail().doEval().get() == evalCrumb);
    return popTailLocation(interp);
}

PLY_INLINE bool isReturnValueOnTopOfStack(Interpreter* interp) {
    return !interp->localVariableStorage.items.isEmpty() &&
           (interp->localVariableStorage.items.tail() == interp->returnValue);
}

StepStatus stepAssign(Interpreter* interp, Breadcrumb::DoAssign* assignCrumb) {
    const Statement::Assignment* assign = assignCrumb->assign;
    for (;;) {
        u32 s = assignCrumb->stage++;
        if (s == 0) {
            if (assign->left->id != Expression::ID::NameLookup) {
                if (startExpression(interp, assign->left) == StepStatus::Completed)
                    return StepStatus::Completed;
            }
        } else if (s == 1) {
            assignCrumb->left = interp->returnValue;
            interp->returnValue = {};
            if (startExpression(interp, assign->right) == StepStatus::Completed)
                return StepStatus::Completed;
        } else {
            PLY_ASSERT(s == 2);
            Interpreter::FunctionStackFrame& stackFrame = interp->stackFrames.back();
            if (assign->left->id == Expression::ID::NameLookup) {
                PLY_ASSERT(!assignCrumb->left.data);
                u32 name = assign->left->nameLookup()->name;
                auto cursor = stackFrame.localVariableTable.insertOrFind(name);
                if (cursor.wasFound()) {
                    cursor->obj.move(interp->returnValue);
                    interp->localVariableStorage.deleteRange(
                        stackFrame.localVariableStorageBoundary,
                        interp->localVariableStorage.items.end());
                } else {
                    if (isReturnValueOnTopOfStack(interp)) {
                        WeakSequenceRef<AnyObject> deleteTo =
                            interp->localVariableStorage.items.end();
                        --deleteTo;
                        interp->localVariableStorage.deleteRange(
                            stackFrame.localVariableStorageBoundary, deleteTo);
                        // Local variable is now located at its permanent address.
                        stackFrame.localVariableStorageBoundary =
                            interp->localVariableStorage.end();
                        cursor->obj = interp->localVariableStorage.items.tail();
                    } else {
                        WeakSequenceRef<AnyObject> deleteTo =
                            interp->localVariableStorage.items.end();
                        interp->localVariableStorage.deleteRange(
                            stackFrame.localVariableStorageBoundary, deleteTo);
                        // Allocate storage for this new local variable.
                        AnyObject* dest =
                            interp->localVariableStorage.appendObject(interp->returnValue.type);
                        dest->move(interp->returnValue);
                        cursor->obj = *dest;
                        stackFrame.localVariableStorageBoundary =
                            interp->localVariableStorage.end();
                    }
                }
                interp->returnValue = {};
            } else {
                assignCrumb->left.move(interp->returnValue);
                interp->returnValue = {};
                interp->localVariableStorage.deleteRange(stackFrame.localVariableStorageBoundary,
                                                         interp->localVariableStorage.items.end());
            }
            PLY_ASSERT(stackFrame.location.tail().doAssign().get() == assignCrumb);
            return popTailLocation(interp);
        }
    }
}

StepStatus stepBinaryOp(Interpreter* interp, Breadcrumb::DoBinaryOp* binaryOpCrumb) {
    const Expression::BinaryOp* binaryOp = binaryOpCrumb->binaryOp;
    for (;;) {
        u32 s = binaryOpCrumb->stage++;
        if (s == 0) {
            if (startExpression(interp, binaryOp->left) == StepStatus::Completed)
                return StepStatus::Completed;
        } else if (s == 1) {
            binaryOpCrumb->left = interp->returnValue;
            interp->returnValue = {};
            if (startExpression(interp, binaryOp->right) == StepStatus::Completed)
                return StepStatus::Completed;
        } else {
            PLY_ASSERT(s == 2);
            interp->returnValue = binaryOpCrumb->left.type->methods.binaryOp(
                &interp->localVariableStorage, binaryOp->op, binaryOpCrumb->left,
                interp->returnValue);
            PLY_ASSERT(interp->stackFrames.back().location.tail().doBinaryOp().get() ==
                       binaryOpCrumb);
            return popTailLocation(interp);
        }
    }
}

void write(OutStream& outs, const AnyObject& arg) {
    if (arg.is<u32>()) {
        outs << *arg.cast<u32>();
    } else if (arg.is<bool>()) {
        outs << *arg.cast<bool>();
    } else if (arg.is<String>()) {
        outs << *arg.cast<String>();
    } else {
        PLY_ASSERT(0);
    }
}

StepStatus stepString(Interpreter* interp, Breadcrumb::DoString* stringCrumb) {
    const Expression::InterpolatedString* stringExp = stringCrumb->string;
    for (;;) {
        u32 s = stringCrumb->stage++;

        u32 argIndex = (s >> 1);
        bool starting = ((s & 1) == 0);
        if (argIndex < stringExp->pieces.numItems()) {
            const auto& piece = stringExp->pieces[argIndex];
            if (starting) {
                stringCrumb->mout << piece.literal;
                if (piece.embed) {
                    if (startExpression(interp, piece.embed) == StepStatus::Completed)
                        return StepStatus::Completed;
                }
            } else {
                if (piece.embed) {
                    write(stringCrumb->mout, interp->returnValue);
                    interp->returnValue = {};
                }
            }
        } else {
            AnyObject* stringObj =
                interp->localVariableStorage.appendObject(getTypeDescriptor<String>());
            *stringObj->cast<String>() = stringCrumb->mout.moveToString();
            interp->returnValue = *stringObj;
            PLY_ASSERT(interp->stackFrames.back().location.tail().doString().get() == stringCrumb);
            return popTailLocation(interp);
        }
    }
}

StepStatus stepCall(Interpreter* interp, Breadcrumb::DoCall* callCrumb) {
    const Expression::Call* call = callCrumb->call;
    for (;;) {
        u32 s = callCrumb->stage++;
        if (s == 0) {
            if (startExpression(interp, call->callable) == StepStatus::Completed)
                return StepStatus::Completed;
        } else if (s == 1) {
            callCrumb->callee = interp->returnValue;
            interp->returnValue = {};
        } else {
            u32 argIndex = (s >> 1) - 1;
            bool starting = ((s & 1) == 0);
            if (argIndex < call->args.numItems()) {
                if (starting) {
                    if (startExpression(interp, call->args[argIndex]) == StepStatus::Completed)
                        return StepStatus::Completed;
                } else {
                    AnyObject* arg;
                    if (!isReturnValueOnTopOfStack(interp)) {
                        arg = interp->localVariableStorage.appendObject(interp->returnValue.type);
                        arg->move(interp->returnValue);
                    } else {
                        arg = &interp->localVariableStorage.items.tail();
                    }
                    callCrumb->args.append(arg);
                    interp->returnValue = {};
                }
            } else {
                if (callCrumb->callee.is<FunctionDefinition>()) {
                    if (starting) {
                        const FunctionDefinition* functionDef =
                            callCrumb->callee.cast<FunctionDefinition>();
                        PLY_ASSERT(callCrumb->args.numItems() ==
                                   functionDef->parameterNames.numItems());
                        HashMap<VariableMapTraits> localVariableTable;
                        for (u32 argIndex : range(callCrumb->args.numItems())) {
                            localVariableTable.insertOrFind(functionDef->parameterNames[argIndex])
                                ->obj = *callCrumb->args[argIndex];
                        }
                        interp->startStackFrame(functionDef->body, &localVariableTable);
                        return StepStatus::Completed;
                    } else {
                        PLY_ASSERT(interp->stackFrames.back().location.tail().doCall().get() ==
                                   callCrumb);
                        return popTailLocation(interp);
                    }
                } else if (auto* funcType =
                               callCrumb->callee.type->cast<TypeDescriptor_Function>()) {
                    PLY_ASSERT(callCrumb->args.numItems() == funcType->paramTypes.numItems());
                    funcType->methods.call(&interp->localVariableStorage, callCrumb->callee);
                    PLY_ASSERT(interp->stackFrames.back().location.tail().doCall().get() ==
                               callCrumb);
                    return popTailLocation(interp);
                } else {
                    PLY_ASSERT(0);
                }
            }
        }
    }
}

StepStatus stepReturn(Interpreter* interp, Breadcrumb::DoReturn* returnCrumb) {
    PLY_ASSERT(interp->stackFrames.back().location.tail().doReturn().get() == returnCrumb);
    destroyStackFrame(interp, true);
    return StepStatus::Completed;
}

StepStatus stepIf(Interpreter* interp, Breadcrumb::DoIf* ifCrumb) {
    const Statement::If_* if_ = ifCrumb->if_;
    for (;;) {
        u32 s = ifCrumb->stage++;
        if (s == 0) {
            if (startExpression(interp, if_->condition) == StepStatus::Completed)
                return StepStatus::Completed;
        } else if (s == 1) {
            // FIXME: Do implicit conversion to bool
            bool wasTrue = (*interp->returnValue.cast<bool>() != 0);
            interp->returnValue = {};
            const StatementBlock* block = (wasTrue ? if_->trueBlock : if_->falseBlock);
            if (block) {
                auto doBlock = interp->stackFrames.back().location.append().doBlock().switchTo();
                doBlock->block = block;
                return StepStatus::Continue;
            }
        } else {
            PLY_ASSERT(s == 2);
            PLY_ASSERT(interp->stackFrames.back().location.tail().doIf().get() == ifCrumb);
            return popTailLocation(interp);
        }
    }
}

StepStatus stepWhile(Interpreter* interp, Breadcrumb::DoWhile* whileCrumb) {
    const Statement::While_* while_ = whileCrumb->while_;
    for (;;) {
        u32 s = whileCrumb->stage++;
        if (s == 0) {
            if (startExpression(interp, while_->condition) == StepStatus::Completed)
                return StepStatus::Completed;
        } else {
            PLY_ASSERT(s == 1);
            // FIXME: Do implicit conversion to bool
            bool wasTrue = (*interp->returnValue.cast<bool>() != 0);
            interp->returnValue = {};
            if (!wasTrue) {
                PLY_ASSERT(interp->stackFrames.back().location.tail().doWhile().get() ==
                           whileCrumb);
                return popTailLocation(interp);
            }
            whileCrumb->stage = 0;
            auto doBlock = interp->stackFrames.back().location.append().doBlock().switchTo();
            doBlock->block = while_->block;
            return StepStatus::Completed;
        }
    }
}

StepStatus startStatement(Interpreter* interp, const Statement* statement) {
    Interpreter::FunctionStackFrame& stackFrame = interp->stackFrames.back();
    switch (statement->id) {
        case Statement::ID::If_: {
            auto ifCrumb = stackFrame.location.append().doIf().switchTo();
            ifCrumb->if_ = statement->if_().get();
            return stepIf(interp, ifCrumb.get());
        }
        case Statement::ID::While_: {
            auto whileCrumb = stackFrame.location.append().doWhile().switchTo();
            whileCrumb->while_ = statement->while_().get();
            return stepWhile(interp, whileCrumb.get());
        }
        case Statement::ID::Evaluate: {
            auto evalCrumb = stackFrame.location.append().doEval().switchTo();
            evalCrumb->eval = statement->evaluate().get();
            return startExpression(interp, statement->evaluate()->expr);
        }
        case Statement::ID::Assignment: {
            auto assignCrumb = stackFrame.location.append().doAssign().switchTo();
            assignCrumb->assign = statement->assignment().get();
            return stepAssign(interp, assignCrumb.get());
        }
        case Statement::ID::Return_: {
            auto returnCrumb = stackFrame.location.append().doReturn().switchTo();
            returnCrumb->return_ = statement->return_().get();
            if (startExpression(interp, statement->return_()->expr) == StepStatus::Completed)
                return StepStatus::Completed;
            return stepReturn(interp, returnCrumb.get());
        }
        default: {
            PLY_ASSERT(0);
            return StepStatus::Completed;
        }
    }
}

StepStatus stepBlock(Interpreter* interp, Breadcrumb::DoBlock* blockCrumb) {
    const StatementBlock* block = blockCrumb->block;

    if (blockCrumb->stage >= block->statements.numItems()) {
        PLY_ASSERT(interp->stackFrames.back().location.tail().doBlock().get() == blockCrumb);
        return popTailLocation(interp);
    }

    u32 s = blockCrumb->stage++;
    return startStatement(interp, block->statements[s]);
}

Interpreter::Interpreter() {
}

void Interpreter::startStackFrame(const StatementBlock* block,
                                  HashMap<VariableMapTraits>* localVariableTable) {
    FunctionStackFrame& stackFrame = this->stackFrames.append();
    stackFrame.endOfPreviousFrameStorage = this->localVariableStorage.end();
    stackFrame.localVariableStorageBoundary = this->localVariableStorage.end();
    if (localVariableTable) {
        stackFrame.localVariableTable = std::move(*localVariableTable);
    }
    auto doBlock = stackFrame.location.append().doBlock().switchTo();
    doBlock->block = block;
}

void Interpreter::step() {
    Interpreter::FunctionStackFrame* stackFrame = &this->stackFrames.back();
    Breadcrumb* crumb = &stackFrame->location.tail();
    switch (crumb->id) {
        case Breadcrumb::ID::DoBlock: {
            stepBlock(this, crumb->doBlock().get());
            return;
        }
        case Breadcrumb::ID::DoEval: {
            stepEval(this, crumb->doEval().get());
            return;
        }
        case Breadcrumb::ID::DoAssign: {
            stepAssign(this, crumb->doAssign().get());
            return;
        }
        case Breadcrumb::ID::DoBinaryOp: {
            stepBinaryOp(this, crumb->doBinaryOp().get());
            return;
        }
        case Breadcrumb::ID::DoCall: {
            stepCall(this, crumb->doCall().get());
            return;
        }
        case Breadcrumb::ID::DoReturn: {
            stepReturn(this, crumb->doReturn().get());
            return;
        }
        case Breadcrumb::ID::DoWhile: {
            stepWhile(this, crumb->doWhile().get());
            return;
        }
        case Breadcrumb::ID::DoIf: {
            stepIf(this, crumb->doIf().get());
            return;
        }
        default: {
            PLY_ASSERT(0);
            return;
        }
    }
}

void Interpreter::finish() {
    while (!this->stackFrames.isEmpty()) {
        this->step();
    }
}

} // namespace crowbar
} // namespace ply

#include "codegen/Interpreter.inl" //%%

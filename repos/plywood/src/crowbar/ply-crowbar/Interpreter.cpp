/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-crowbar/Core.h>
#include <ply-crowbar/Interpreter.h>

namespace ply {
namespace crowbar {

enum class Result {
    Return,
    Continue,
};

void eval(Interpreter::StackFrame* frame, const Expression* expr);
Result execBlock(Interpreter::StackFrame* frame, const StatementBlock* block);
void execFunction(Interpreter::StackFrame* frame, const StatementBlock* block);

// FIXME: Generalize this:
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

void evalString(Interpreter::StackFrame* frame, const Expression::InterpolatedString* stringExp) {
    Interpreter* interp = frame->interp;

    MemOutStream mout;
    for (const Expression::InterpolatedString::Piece& piece : stringExp->pieces) {
        mout << piece.literal;
        if (piece.embed) {
            eval(frame, piece.embed);
            write(mout, interp->returnValue);
            interp->returnValue = {};
        }
    }

    // Return string with all substitutions performed.
    AnyObject* stringObj = interp->localVariableStorage.appendObject(getTypeDescriptor<String>());
    *stringObj->cast<String>() = mout.moveToString();
    interp->returnValue = *stringObj;
}

PLY_INLINE bool isReturnValueOnTopOfStack(Interpreter* interp) {
    return !interp->localVariableStorage.items.isEmpty() &&
           (interp->localVariableStorage.items.tail() == interp->returnValue);
}

void evalPropertyLookup(Interpreter::StackFrame* frame,
                        const Expression::PropertyLookup* propLookup) {
    Interpreter* interp = frame->interp;

    // Evaluate left side.
    eval(frame, propLookup->obj);
    AnyObject obj = interp->returnValue;
    interp->returnValue = {};

    // Perform property lookup.
    // FIXME: This should accept interned strings.
    interp->returnValue =
        obj.type->methods.propertyLookup(&interp->localVariableStorage, obj,
                                         interp->internedStrings->view(propLookup->propertyName));
}

void evalBinaryOp(Interpreter::StackFrame* frame, const Expression::BinaryOp* binaryOp) {
    Interpreter* interp = frame->interp;

    // Evaluate left side.
    eval(frame, binaryOp->left);
    AnyObject left = interp->returnValue;
    interp->returnValue = {};

    // Evaluate right side.
    eval(frame, binaryOp->right);
    AnyObject right = interp->returnValue;
    interp->returnValue = {};

    // Invoke operator.
    interp->returnValue =
        left.type->methods.binaryOp(&interp->localVariableStorage, binaryOp->op, left, right);
}

void evalCall(Interpreter::StackFrame* frame, const Expression::Call* call) {
    Interpreter* interp = frame->interp;

    // Evaluate callable.
    eval(frame, call->callable);
    AnyObject callee = interp->returnValue;
    interp->returnValue = {};

    // Evaluate arguments.
    Array<AnyObject*> args;
    for (const Expression* argExpr : call->args) {
        eval(frame, argExpr);

        AnyObject* arg;
        if (!isReturnValueOnTopOfStack(interp)) {
            arg = interp->localVariableStorage.appendObject(interp->returnValue.type);
            arg->move(interp->returnValue);
        } else {
            arg = &interp->localVariableStorage.items.tail();
        }
        args.append(arg);
        interp->returnValue = {};
    }

    // Invoke function using provided arguments.
    if (callee.is<FunctionDefinition>()) {
        // It's a Crowbar function.
        const FunctionDefinition* functionDef = callee.cast<FunctionDefinition>();
        PLY_ASSERT(args.numItems() == functionDef->parameterNames.numItems());

        // Set up a new stack frame.
        Interpreter::StackFrame frame;
        frame.interp = interp;
        for (u32 argIndex : range(args.numItems())) {
            frame.localVariableTable.insertOrFind(functionDef->parameterNames[argIndex])->obj =
                *args[argIndex];
        }

        // Execute function body and clean up stack frame.
        execFunction(&frame, functionDef->body);
    } else if (auto* funcType = callee.type->cast<TypeDescriptor_Function>()) {
        // It's a native function.
        PLY_ASSERT(args.numItems() == funcType->paramTypes.numItems());
        funcType->methods.call(&interp->localVariableStorage, callee);
    } else {
        PLY_ASSERT(0);
    }
}

AnyObject lookupName(Interpreter::StackFrame* frame, u32 name) {
    Interpreter* interp = frame->interp;

    auto cursor = frame->localVariableTable.find(name);
    if (cursor.wasFound())
        return cursor->obj;

    for (s32 i = interp->outerNameSpaces.numItems() - 1; i >= 0; i--) {
        const HashMap<VariableMapTraits>* ns = interp->outerNameSpaces[i];
        auto cursor = ns->find(name);
        if (cursor.wasFound())
            return cursor->obj;
    }

    return {};
}

void eval(Interpreter::StackFrame* frame, const Expression* expr) {
    Interpreter* interp = frame->interp;

    switch (expr->id) {
        case Expression::ID::NameLookup: {
            interp->returnValue = lookupName(frame, expr->nameLookup()->name);
            PLY_ASSERT(interp->returnValue.data);
            return;
        }

        case Expression::ID::IntegerLiteral: {
            interp->returnValue = AnyObject::bind(&expr->integerLiteral()->value);
            return;
        }

        case Expression::ID::InterpolatedString: {
            evalString(frame, expr->interpolatedString().get());
            return;
        }

        case Expression::ID::PropertyLookup: {
            evalPropertyLookup(frame, expr->propertyLookup().get());
            return;
        }

        case Expression::ID::BinaryOp: {
            evalBinaryOp(frame, expr->binaryOp().get());
            return;
        }

        case Expression::ID::Call: {
            evalCall(frame, expr->call().get());
            return;
        }

        default: {
            PLY_ASSERT(0);
            return;
        }
    }
}

Result execIf(Interpreter::StackFrame* frame, const Statement::If_* if_) {
    Interpreter* interp = frame->interp;

    // Evaluate condition.
    ObjectStack::Boundary localVariableStorageBoundary = interp->localVariableStorage.end();
    eval(frame, if_->condition);
    // FIXME: Do implicit conversion to bool
    bool wasTrue = (*interp->returnValue.cast<bool>() != 0);
    interp->returnValue = {};
    // Delete temporary objects.
    interp->localVariableStorage.deleteRange(localVariableStorageBoundary,
                                             interp->localVariableStorage.items.end());

    // Execute the appropriate child block (if any).
    const StatementBlock* block = (wasTrue ? if_->trueBlock : if_->falseBlock);
    if (block) {
        if (execBlock(frame, block) == Result::Return)
            return Result::Return;
    }
    return Result::Continue;
}

Result execWhile(Interpreter::StackFrame* frame, const Statement::While_* while_) {
    Interpreter* interp = frame->interp;

    for (;;) {
        // Evaluate condition.
        ObjectStack::Boundary localVariableStorageBoundary = interp->localVariableStorage.end();
        eval(frame, while_->condition);
        // FIXME: Do implicit conversion to bool
        bool wasTrue = (*interp->returnValue.cast<bool>() != 0);
        interp->returnValue = {};
        // Delete temporary objects.
        interp->localVariableStorage.deleteRange(localVariableStorageBoundary,
                                                 interp->localVariableStorage.items.end());

        // Either stop, or execute the child block.
        if (!wasTrue)
            return Result::Continue;
        if (execBlock(frame, while_->block) == Result::Return)
            return Result::Return;
    }
}

void execAssign(Interpreter::StackFrame* frame, const Statement::Assignment* assign) {
    Interpreter* interp = frame->interp;
    ObjectStack::Boundary localVariableStorageBoundary = interp->localVariableStorage.end();

    // Evaluate left side.
    AnyObject left;
    if (assign->left->id != Expression::ID::NameLookup) {
        eval(frame, assign->left);
        left = interp->returnValue;
        interp->returnValue = {};
    }

    // Evaluate right side.
    eval(frame, assign->right);

    // Perform assignment.
    if (assign->left->id == Expression::ID::NameLookup) {
        PLY_ASSERT(!left.data);
        u32 name = assign->left->nameLookup()->name;
        auto cursor = frame->localVariableTable.insertOrFind(name);
        if (cursor.wasFound()) {
            // Move result to existing local variable.
            cursor->obj.move(interp->returnValue);
            // Delete temporary objects.
            interp->localVariableStorage.deleteRange(localVariableStorageBoundary,
                                                     interp->localVariableStorage.items.end());
        } else {
            if (isReturnValueOnTopOfStack(interp)) {
                WeakSequenceRef<AnyObject> deleteTo = interp->localVariableStorage.items.end();
                --deleteTo;
                // Delete temporary objects except for the return value.
                interp->localVariableStorage.deleteRange(localVariableStorageBoundary, deleteTo);
                // We've just created a new local Local variable.
                cursor->obj = interp->localVariableStorage.items.tail();
            } else {
                // Delete temporary objects.
                interp->localVariableStorage.deleteRange(localVariableStorageBoundary,
                                                         interp->localVariableStorage.items.end());
                // Allocate storage for new local variable.
                AnyObject* dest =
                    interp->localVariableStorage.appendObject(interp->returnValue.type);
                dest->move(interp->returnValue);
                cursor->obj = *dest;
            }
        }
        interp->returnValue = {};
    } else {
        left.move(interp->returnValue);
        interp->returnValue = {};
        // Delete temporary objects.
        interp->localVariableStorage.deleteRange(localVariableStorageBoundary,
                                                 interp->localVariableStorage.items.end());
    }
}

Result execBlock(Interpreter::StackFrame* frame, const StatementBlock* block) {
    Interpreter* interp = frame->interp;

    // Execute each statement in this block.
    for (const Statement* statement : block->statements) {
        switch (statement->id) {
            case Statement::ID::If_: {
                if (execIf(frame, statement->if_().get()) == Result::Return)
                    return Result::Return;
                break;
            }
            case Statement::ID::While_: {
                if (execWhile(frame, statement->while_().get()) == Result::Return)
                    return Result::Return;
                break;
            }
            case Statement::ID::Assignment: {
                execAssign(frame, statement->assignment().get());
                break;
            }
            case Statement::ID::Evaluate: {
                ObjectStack::Boundary localVariableStorageBoundary =
                    interp->localVariableStorage.end();
                eval(frame, statement->evaluate()->expr);
                interp->returnValue = {};
                // Delete temporary objects.
                interp->localVariableStorage.deleteRange(localVariableStorageBoundary,
                                                         interp->localVariableStorage.items.end());
                break;
            }
            case Statement::ID::Return_: {
                eval(frame, statement->return_()->expr);
                return Result::Return;
            }
            default: {
                PLY_ASSERT(0);
                return Result::Continue;
            }
        }
    }

    return Result::Continue;
}

void execFunction(Interpreter::StackFrame* frame, const StatementBlock* block) {
    Interpreter* interp = frame->interp;
    ObjectStack::Boundary endOfPreviousFrameStorage = interp->localVariableStorage.end();

    // Execute function body.
    execBlock(frame, block);

    // Destroy all local variables in this stack frame.
    WeakSequenceRef<AnyObject> deleteTo = interp->localVariableStorage.items.end();
    bool fixupReturnValue = (endOfPreviousFrameStorage != interp->localVariableStorage.end()) &&
                            (interp->localVariableStorage.items.tail() == interp->returnValue);
    if (fixupReturnValue) {
        --deleteTo;
    }
    interp->localVariableStorage.deleteRange(endOfPreviousFrameStorage, deleteTo);
    if (fixupReturnValue) {
        PLY_ASSERT(interp->localVariableStorage.items.tail().type == interp->returnValue.type);
        interp->returnValue.data = interp->localVariableStorage.items.tail().data;
    }
}

} // namespace crowbar
} // namespace ply

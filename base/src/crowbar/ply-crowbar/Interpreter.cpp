/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-crowbar/Core.h>
#include <ply-crowbar/Interpreter.h>

namespace ply {
namespace crowbar {

Functor<HybridString()> makeFunctionDesc(const Statement::FunctionDefinition* fnDef) {
    return [fnDef]() -> HybridString {
        return String::format("function '{}'", g_labelStorage.view(fnDef->name));
    };
}

void error(const Interpreter* interp, StringView message) {
    MemOutStream outs;
    outs.format("error: {}\n", message);
    bool first = true;
    for (Interpreter::StackFrame* frame = interp->currentFrame; frame; frame = frame->prevFrame) {
        ExpandedToken expToken = frame->tkr->expandToken(frame->tokenIdx);
        outs.format("{} {} {}\n",
                    frame->tkr->fileLocationMap.formatFileLocation(expToken.fileOffset),
                    first ? "in" : "called from", frame->desc());
        first = false;
    }
    interp->base.error(outs.moveToString());
}

AnyObject find(const LabelMap<AnyObject>& map, Label identifier) {
    const AnyObject* value = map.find(identifier);
    return value ? *value : AnyObject{};
}

MethodResult execBlock(Interpreter::StackFrame* frame, const StatementBlock* block);
MethodResult execFunction(Interpreter::StackFrame* frame, const StatementBlock* block);

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

MethodResult evalString(Interpreter::StackFrame* frame,
                        const Expression::InterpolatedString* stringExp) {
    BaseInterpreter* base = &frame->interp->base;

    MemOutStream mout;
    for (const Expression::InterpolatedString::Piece& piece : stringExp->pieces) {
        mout << piece.literal;
        if (piece.embed) {
            MethodResult result = eval(frame, piece.embed);
            if (result != MethodResult::OK)
                return result;
            write(mout, base->returnValue);
            base->returnValue = {};
        }
    }

    // Return string with all substitutions performed.
    AnyObject* stringObj = base->localVariableStorage.appendObject(getTypeDescriptor<String>());
    *stringObj->cast<String>() = mout.moveToString();
    base->returnValue = *stringObj;
    return MethodResult::OK;
}

PLY_INLINE bool isReturnValueOnTopOfStack(BaseInterpreter* base) {
    return !base->localVariableStorage.items.isEmpty() &&
           (base->localVariableStorage.items.tail() == base->returnValue);
}

MethodResult evalPropertyLookup(Interpreter::StackFrame* frame,
                                const Expression::PropertyLookup* propLookup) {
    BaseInterpreter* base = &frame->interp->base;

    // Evaluate left side.
    MethodResult result = eval(frame, propLookup->obj);
    if (result != MethodResult::OK)
        return result;
    AnyObject obj = base->returnValue;
    base->returnValue = {};

    // Perform property lookup.
    // FIXME: This should accept interned strings.
    return obj.type->methods.propertyLookup(base, obj,
                                            g_labelStorage.view(propLookup->propertyName));
}

MethodResult evalBinaryOp(Interpreter::StackFrame* frame, const Expression::BinaryOp* binaryOp) {
    BaseInterpreter* base = &frame->interp->base;

    // Evaluate left side.
    MethodResult result = eval(frame, binaryOp->left);
    if (result != MethodResult::OK)
        return result;
    AnyObject left = base->returnValue;
    base->returnValue = {};

    // Evaluate right side.
    result = eval(frame, binaryOp->right);
    if (result != MethodResult::OK)
        return result;
    AnyObject right = base->returnValue;
    base->returnValue = {};

    // Invoke operator.
    return left.type->methods.binaryOp(base, binaryOp->op, left, right);
}

MethodResult evalUnaryOp(Interpreter::StackFrame* frame, const Expression::UnaryOp* unaryOp) {
    BaseInterpreter* base = &frame->interp->base;

    // Evaluate subexpression.
    MethodResult result = eval(frame, unaryOp->expr);
    if (result != MethodResult::OK)
        return result;
    AnyObject obj = base->returnValue;
    base->returnValue = {};

    // Invoke operator.
    return obj.type->methods.unaryOp(base, unaryOp->op, obj);
}

MethodResult evalCall(Interpreter::StackFrame* frame, const Expression::Call* call) {
    BaseInterpreter* base = &frame->interp->base;

    // Evaluate callable.
    MethodResult result = eval(frame, call->callable);
    if (result != MethodResult::OK)
        return result;
    AnyObject callee = base->returnValue;
    base->returnValue = {};

    // Evaluate arguments.
    Array<AnyObject> args;
    for (const Expression* argExpr : call->args) {
        result = eval(frame, argExpr);
        if (result != MethodResult::OK)
            return result;

        AnyObject* arg;
        if (!isReturnValueOnTopOfStack(base)) {
            // The return value is not a temporary object, and the interpreter is currently designed
            // to pass all arguments "by value", like in C, so we should make a copy here. (In the
            // future, we could extend the interpreter to support passing "by reference" as well,
            // like in C++. In the meantime, scripts can achieve the same thing by passing
            // pointers.)
            arg = base->localVariableStorage.appendObject(base->returnValue.type);
            arg->copy(base->returnValue);
        } else {
            arg = &base->localVariableStorage.items.tail();
        }
        args.append(*arg);
        base->returnValue = {};
    }

    // Invoke function using provided arguments.
    if (callee.is<Statement::FunctionDefinition>()) {
        // It's a Crowbar function.
        // FIXME: Move this to MethodTable.
        const auto* functionDef = callee.cast<Statement::FunctionDefinition>();
        PLY_ASSERT(args.numItems() == functionDef->parameterNames.numItems());

        // Set up a new stack frame.
        Interpreter::StackFrame newFrame;
        newFrame.interp = frame->interp;
        newFrame.desc = makeFunctionDesc(functionDef);
        newFrame.tkr = functionDef->tkr;
        newFrame.prevFrame = frame;
        for (u32 argIndex : range(args.numItems())) {
            AnyObject* value;
            newFrame.localVariableTable.insertOrFind(functionDef->parameterNames[argIndex], &value);
            *value = args[argIndex];
        }

        // Execute function body and clean up stack frame.
        return execFunction(&newFrame, functionDef->body);
    } else {
        // Call through MethodTable.
        return callee.type->methods.call(base, callee, args);
    }
}

AnyObject lookupName(Interpreter::StackFrame* frame, Label name) {
    // Check local variables first.
    AnyObject* value = frame->localVariableTable.find(name);
    if (value)
        return *value;

    // Then builtins.
    value = frame->interp->builtIns.find(name);
    if (value)
        return *value;

    // Then dynamic hooks.
    return frame->interp->hooks.resolveName(name);
}

MethodResult eval(Interpreter::StackFrame* frame, const Expression* expr) {
    BaseInterpreter* base = &frame->interp->base;
    PLY_SET_IN_SCOPE(frame->tokenIdx, expr->tokenIdx);

    switch (expr->id) {
        case Expression::ID::NameLookup: {
            base->returnValue = lookupName(frame, expr->nameLookup()->name);
            if (!base->returnValue.data) {
                error(frame->interp, String::format("cannot resolve identifier '{}'",
                                                    g_labelStorage.view(expr->nameLookup()->name)));
                return MethodResult::Error;
            }
            return MethodResult::OK;
        }

        case Expression::ID::IntegerLiteral: {
            base->returnValue = AnyObject::bind(&expr->integerLiteral()->value);
            return MethodResult::OK;
        }

        case Expression::ID::InterpolatedString: {
            return evalString(frame, expr->interpolatedString().get());
        }

        case Expression::ID::PropertyLookup: {
            return evalPropertyLookup(frame, expr->propertyLookup().get());
        }

        case Expression::ID::BinaryOp: {
            return evalBinaryOp(frame, expr->binaryOp().get());
        }

        case Expression::ID::UnaryOp: {
            return evalUnaryOp(frame, expr->unaryOp().get());
        }

        case Expression::ID::Call: {
            return evalCall(frame, expr->call().get());
        }

        default: {
            PLY_ASSERT(0);
            return MethodResult::Error;
        }
    }
}

MethodResult execIf(Interpreter::StackFrame* frame, const Statement::If_* if_) {
    BaseInterpreter* base = &frame->interp->base;

    // Evaluate condition.
    ObjectStack::Boundary localVariableStorageBoundary = base->localVariableStorage.end();
    MethodResult result = eval(frame, if_->condition);
    if (result != MethodResult::OK)
        return result;
    // FIXME: Do implicit conversion to bool
    bool wasTrue = (*base->returnValue.cast<bool>() != 0);
    base->returnValue = {};
    // Delete temporary objects.
    base->localVariableStorage.deleteRange(localVariableStorageBoundary,
                                           base->localVariableStorage.items.end());

    // Execute the appropriate child block (if any).
    PLY_ASSERT(if_->trueBlock);
    const StatementBlock* block = (wasTrue ? if_->trueBlock : if_->falseBlock);
    if (block)
        return execBlock(frame, block);
    return MethodResult::OK;
}

MethodResult execWhile(Interpreter::StackFrame* frame, const Statement::While_* while_) {
    BaseInterpreter* base = &frame->interp->base;

    for (;;) {
        // Evaluate condition.
        ObjectStack::Boundary localVariableStorageBoundary = base->localVariableStorage.end();
        MethodResult result = eval(frame, while_->condition);
        if (result != MethodResult::OK)
            return result;
        // FIXME: Do implicit conversion to bool
        bool wasTrue = (*base->returnValue.cast<bool>() != 0);
        base->returnValue = {};
        // Delete temporary objects.
        base->localVariableStorage.deleteRange(localVariableStorageBoundary,
                                               base->localVariableStorage.items.end());

        // Either stop, or execute the child block.
        if (!wasTrue)
            return MethodResult::OK;
        result = execBlock(frame, while_->block);
        if (result != MethodResult::OK)
            return result;
    }
}

MethodResult execAssign(Interpreter::StackFrame* frame, const Statement::Assignment* assign) {
    BaseInterpreter* base = &frame->interp->base;
    ObjectStack::Boundary localVariableStorageBoundary = base->localVariableStorage.end();

    // Evaluate left side.
    AnyObject left;
    if (assign->left->id != Expression::ID::NameLookup) {
        MethodResult result = eval(frame, assign->left);
        if (result != MethodResult::OK)
            return result;
        left = base->returnValue;
        base->returnValue = {};
    }

    // Evaluate right side.
    MethodResult result = eval(frame, assign->right);
    if (result != MethodResult::OK)
        return result;

    // Perform assignment.
    if (assign->left->id == Expression::ID::NameLookup) {
        PLY_ASSERT(!left.data);
        Label name = assign->left->nameLookup()->name;
        if (frame->interp->hooks.assignToLocal(name))
            return MethodResult::OK;

        AnyObject* value;
        if (frame->localVariableTable.insertOrFind(name, &value)) {
            if (isReturnValueOnTopOfStack(base)) {
                WeakSequenceRef<AnyObject> deleteTo = base->localVariableStorage.items.end();
                --deleteTo;
                // Delete temporary objects except for the return value.
                base->localVariableStorage.deleteRange(localVariableStorageBoundary, deleteTo);
                // We've just created a new local Local variable.
                *value = base->localVariableStorage.items.tail();
            } else {
                // Delete temporary objects.
                base->localVariableStorage.deleteRange(localVariableStorageBoundary,
                                                       base->localVariableStorage.items.end());
                // Allocate storage for new local variable.
                AnyObject* dest = base->localVariableStorage.appendObject(base->returnValue.type);
                dest->move(base->returnValue);
                *value = *dest;
            }
        } else {
            // Move result to existing local variable.
            value->move(base->returnValue);
            // Delete temporary objects.
            base->localVariableStorage.deleteRange(localVariableStorageBoundary,
                                                   base->localVariableStorage.items.end());
        }
        base->returnValue = {};
    } else {
        left.move(base->returnValue);
        base->returnValue = {};
        // Delete temporary objects.
        base->localVariableStorage.deleteRange(localVariableStorageBoundary,
                                               base->localVariableStorage.items.end());
    }
    return MethodResult::OK;
}

MethodResult execBlock(Interpreter::StackFrame* frame, const StatementBlock* block) {
    BaseInterpreter* base = &frame->interp->base;

    // Execute each statement in this block.
    for (const Statement* statement : block->statements) {
        frame->tokenIdx = statement->tokenIdx;
        switch (statement->id) {
            case Statement::ID::If_: {
                MethodResult result = execIf(frame, statement->if_().get());
                if (result != MethodResult::OK)
                    return result;
                break;
            }
            case Statement::ID::While_: {
                MethodResult result = execWhile(frame, statement->while_().get());
                if (result != MethodResult::OK)
                    return result;
                break;
            }
            case Statement::ID::Assignment: {
                MethodResult result = execAssign(frame, statement->assignment().get());
                if (result != MethodResult::OK)
                    return result;
                break;
            }
            case Statement::ID::Evaluate: {
                ObjectStack::Boundary localVariableStorageBoundary =
                    base->localVariableStorage.end();
                MethodResult result = eval(frame, statement->evaluate()->expr);
                if (result != MethodResult::OK)
                    return result;
                frame->interp->hooks.onEvaluate(statement->evaluate()->traits);
                base->returnValue = {};
                // Delete temporary objects.
                base->localVariableStorage.deleteRange(localVariableStorageBoundary,
                                                       base->localVariableStorage.items.end());
                break;
            }
            case Statement::ID::Return_: {
                MethodResult result = eval(frame, statement->return_()->expr);
                if (result != MethodResult::OK)
                    return result;
                return MethodResult::Return;
            }
            case Statement::ID::CustomBlock: {
                const Statement::CustomBlock* cb = statement->customBlock().get();
                frame->interp->hooks.customBlock(cb, true);
                MethodResult result;
                {
                    PLY_SET_IN_SCOPE(frame->interp->currentFrame->customBlock, cb);
                    result = execBlock(frame, cb->body);
                }
                frame->interp->hooks.customBlock(cb, false);
                if (result != MethodResult::OK)
                    return result;
                break;
            }
            default: {
                PLY_ASSERT(0);
                return MethodResult::Error;
            }
        }
    }

    return MethodResult::OK;
}

MethodResult execFunction(Interpreter::StackFrame* frame, const StatementBlock* block) {
    BaseInterpreter* base = &frame->interp->base;
    PLY_SET_IN_SCOPE(frame->interp->currentFrame, frame);
    ObjectStack::Boundary endOfPreviousFrameStorage = base->localVariableStorage.end();

    // Execute function body.
    MethodResult result = execBlock(frame, block);
    if (result == MethodResult::Return)
        result = MethodResult::OK;

    // Destroy all local variables in this stack frame.
    WeakSequenceRef<AnyObject> deleteTo = base->localVariableStorage.items.end();
    bool fixupReturnValue = (endOfPreviousFrameStorage != base->localVariableStorage.end()) &&
                            (base->localVariableStorage.items.tail() == base->returnValue);
    if (fixupReturnValue) {
        --deleteTo;
    }
    base->localVariableStorage.deleteRange(endOfPreviousFrameStorage, deleteTo);
    if (fixupReturnValue) {
        PLY_ASSERT(base->localVariableStorage.items.tail().type == base->returnValue.type);
        base->returnValue.data = base->localVariableStorage.items.tail().data;
    }

    return result;
}

} // namespace crowbar
} // namespace ply

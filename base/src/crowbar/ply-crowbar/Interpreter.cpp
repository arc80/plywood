/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-crowbar/Core.h>
#include <ply-crowbar/Interpreter.h>
#include <ply-reflect/methods/BoundMethod.h>

namespace ply {
namespace crowbar {

void logErrorWithStack(OutStream* outs, const Interpreter* interp, StringView message) {
    Interpreter::StackFrame* frame = interp->currentFrame;
    ExpandedToken expToken = frame->tkr->expandToken(frame->tokenIdx);
    outs->format("{} error: {}\n",
                 frame->tkr->fileLocationMap.formatFileLocation(expToken.fileOffset), message);
    for (;;) {
        frame = frame->prevFrame;
        if (!frame)
            break;
        ExpandedToken expToken = frame->tkr->expandToken(frame->tokenIdx);
        outs->format("{}: called from {}\n",
                     frame->tkr->fileLocationMap.formatFileLocation(expToken.fileOffset),
                     frame->desc());
    }
}

AnyObject find(const LabelMap<AnyObject>& map, Label identifier) {
    const AnyObject* value = map.find(identifier);
    return value ? *value : AnyObject{};
}

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

    // Handle bound methods.
    AnyObject self;
    if (BoundMethod* bm = callee.safeCast<BoundMethod>()) {
        self = bm->target;
        callee = bm->func;
    }

    // Invoke the callee with with provided arguments.
    if (const auto* functionDef = callee.safeCast<Statement::FunctionDefinition>()) {
        // Function is implemented in script.
        PLY_ASSERT(!self); // Not possible to define classes in script yet
        PLY_ASSERT(args.numItems() == functionDef->parameterNames.numItems());

        // Set up a new stack frame.
        Interpreter::StackFrame newFrame;
        newFrame.interp = frame->interp;
        newFrame.desc = [functionDef]() -> HybridString {
            return String::format("function '{}'", g_labelStorage.view(functionDef->name));
        };
        newFrame.tkr = functionDef->tkr;
        newFrame.prevFrame = frame;
        for (u32 argIndex : range(args.numItems())) {
            AnyObject* value;
            newFrame.localVariableTable.insertOrFind(functionDef->parameterNames[argIndex], &value);
            *value = args[argIndex];
        }

        // Execute function body and clean up stack frame.
        return execFunction(&newFrame, functionDef->body);
    } else if (Method* method = callee.safeCast<Method>()) {
        // Function is implemented in C++.
        MethodArgs ma;
        ma.base = base;
        ma.self = self;
        ma.args = args;
        return method(ma);
    }

    // Object is not callable
    base->error(String::format("cannot call '{}' as a function", callee.type->getName()));
    return MethodResult::Error;
}

AnyObject lookupName(Interpreter::StackFrame* frame, Label name) {
    // Check local variables first.
    AnyObject* value = frame->localVariableTable.find(name);
    if (value)
        return *value;

    // Then client.
    return frame->interp->resolveName(name);
}

MethodResult eval(Interpreter::StackFrame* frame, const Expression* expr) {
    BaseInterpreter* base = &frame->interp->base;
    PLY_SET_IN_SCOPE(frame->tokenIdx, expr->tokenIdx);

    switch (expr->id) {
        case Expression::ID::NameLookup: {
            base->returnValue = lookupName(frame, expr->nameLookup()->name);
            if (!base->returnValue.data) {
                base->error(String::format("cannot resolve identifier '{}'",
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
        if (frame->hooks.assignToLocal(assign->attributes, name))
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
                bool hookResult = frame->hooks.onEvaluate(statement->evaluate()->attributes);
                base->returnValue = {};
                // Delete temporary objects.
                base->localVariableStorage.deleteRange(localVariableStorageBoundary,
                                                       base->localVariableStorage.items.end());
                if (!hookResult)
                    return MethodResult::Error;
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
                MethodResult result = frame->hooks.doCustomBlock(cb);
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

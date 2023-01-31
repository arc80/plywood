/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-biscuit/Core.h>
#include <ply-biscuit/Interpreter.h>
#include <ply-reflect/methods/BoundMethod.h>

namespace ply {
namespace biscuit {

void logErrorWithStack(OutStream& out, const Interpreter* interp, StringView message) {
    Interpreter::StackFrame* frame = interp->currentFrame;
    ExpandedToken expToken = frame->tkr->expandToken(frame->tokenIdx);
    out.format("{} error: {}\n",
                 frame->tkr->fileLocationMap.formatFileLocation(expToken.fileOffset), message);
    for (;;) {
        frame = frame->prevFrame;
        if (!frame)
            break;
        ExpandedToken expToken = frame->tkr->expandToken(frame->tokenIdx);
        out.format("{}: called from {}\n",
                     frame->tkr->fileLocationMap.formatFileLocation(expToken.fileOffset),
                     frame->desc());
    }
}

AnyObject find(const LabelMap<AnyObject>& map, Label identifier) {
    const AnyObject* value = map.find(identifier);
    return value ? *value : AnyObject{};
}

FnResult execFunction(Interpreter::StackFrame* frame, const StatementBlock* block);

// FIXME: Generalize this:
void write(OutStream& out, const AnyObject& arg) {
    if (arg.is<u32>()) {
        out << *arg.cast<u32>();
    } else if (arg.is<bool>()) {
        out << *arg.cast<bool>();
    } else if (arg.is<String>()) {
        out << *arg.cast<String>();
    } else {
        PLY_ASSERT(0);
    }
}

FnResult evalString(Interpreter::StackFrame* frame,
                        const Expression::InterpolatedString* stringExp) {
    BaseInterpreter* base = &frame->interp->base;

    MemOutStream mout;
    for (const Expression::InterpolatedString::Piece& piece : stringExp->pieces) {
        mout << piece.literal;
        if (piece.embed) {
            FnResult result = eval(frame, piece.embed);
            if (result != Fn_OK)
                return result;
            write(mout, base->returnValue);
            base->returnValue = {};
        }
    }

    // Return string with all substitutions performed.
    AnyObject* stringObj = base->localVariableStorage.appendObject(getTypeDescriptor<String>());
    *stringObj->cast<String>() = mout.moveToString();
    base->returnValue = *stringObj;
    return Fn_OK;
}

PLY_INLINE bool isReturnValueOnTopOfStack(BaseInterpreter* base) {
    return !base->localVariableStorage.items.isEmpty() &&
           (base->localVariableStorage.items.tail() == base->returnValue);
}

FnResult evalPropertyLookup(Interpreter::StackFrame* frame,
                                const Expression::PropertyLookup* propLookup) {
    BaseInterpreter* base = &frame->interp->base;

    // Evaluate left side.
    FnResult result = eval(frame, propLookup->obj);
    if (result != Fn_OK)
        return result;
    AnyObject obj = base->returnValue;
    base->returnValue = {};

    // Perform property lookup.
    // FIXME: This should accept interned strings.
    return obj.type->methods.propertyLookup(base, obj,
                                            g_labelStorage.view(propLookup->propertyName));
}

FnResult evalBinaryOp(Interpreter::StackFrame* frame, const Expression::BinaryOp* binaryOp) {
    BaseInterpreter* base = &frame->interp->base;

    // Evaluate left side.
    FnResult result = eval(frame, binaryOp->left);
    if (result != Fn_OK)
        return result;
    AnyObject left = base->returnValue;
    base->returnValue = {};

    // Evaluate right side.
    result = eval(frame, binaryOp->right);
    if (result != Fn_OK)
        return result;
    AnyObject right = base->returnValue;
    base->returnValue = {};

    // Invoke operator.
    return left.type->methods.binaryOp(base, binaryOp->op, left, right);
}

FnResult evalUnaryOp(Interpreter::StackFrame* frame, const Expression::UnaryOp* unaryOp) {
    BaseInterpreter* base = &frame->interp->base;

    // Evaluate subexpression.
    FnResult result = eval(frame, unaryOp->expr);
    if (result != Fn_OK)
        return result;
    AnyObject obj = base->returnValue;
    base->returnValue = {};

    // Invoke operator.
    return obj.type->methods.unaryOp(base, unaryOp->op, obj);
}

FnResult evalCall(Interpreter::StackFrame* frame, const Expression::Call* call) {
    BaseInterpreter* base = &frame->interp->base;

    // Evaluate callable.
    FnResult result = eval(frame, call->callable);
    if (result != Fn_OK)
        return result;
    AnyObject callee = base->returnValue;
    base->returnValue = {};

    // Evaluate arguments.
    Array<AnyObject> args;
    for (const Expression* argExpr : call->args) {
        result = eval(frame, argExpr);
        if (result != Fn_OK)
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

    // Invoke the callee with the provided arguments.
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
        for (u32 argIndex = 0; argIndex < args.numItems(); argIndex++) {
            AnyObject* value;
            newFrame.localVariableTable.insertOrFind(functionDef->parameterNames[argIndex], &value);
            *value = args[argIndex];
        }

        // Execute function body and clean up stack frame.
        return execFunction(&newFrame, functionDef->body);
    } else if (NativeFunction* func = callee.safeCast<NativeFunction>()) {
        FnParams params;
        params.base = base;
        params.self = self;
        params.args = args;
        return func(params);
    } else if (BoundNativeMethod* bnm = callee.safeCast<BoundNativeMethod>()) {
        FnParams params;
        params.base = base;
        params.self = self;
        params.args = args;
        return bnm->func(bnm->self, params);
    }

    // Object is not callable
    base->error(String::format("cannot call '{}' as a function", callee.type->getName()));
    return Fn_Error;
}

AnyObject lookupName(Interpreter::StackFrame* frame, Label name) {
    // Check local variables first.
    AnyObject* value = frame->localVariableTable.find(name);
    if (value)
        return *value;

    // Then client.
    return frame->interp->resolveName(name);
}

FnResult eval(Interpreter::StackFrame* frame, const Expression* expr) {
    BaseInterpreter* base = &frame->interp->base;
    PLY_SET_IN_SCOPE(frame->tokenIdx, expr->tokenIdx);

    switch (expr->id) {
        case Expression::ID::NameLookup: {
            base->returnValue = lookupName(frame, expr->nameLookup()->name);
            if (!base->returnValue.data) {
                base->error(String::format("cannot resolve identifier '{}'",
                                           g_labelStorage.view(expr->nameLookup()->name)));
                return Fn_Error;
            }
            return Fn_OK;
        }

        case Expression::ID::IntegerLiteral: {
            base->returnValue = AnyObject::bind(&expr->integerLiteral()->value);
            return Fn_OK;
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
            return Fn_Error;
        }
    }
}

FnResult execIf(Interpreter::StackFrame* frame, const Statement::If_* if_) {
    BaseInterpreter* base = &frame->interp->base;

    // Evaluate condition.
    ObjectStack::Boundary localVariableStorageBoundary = base->localVariableStorage.end();
    FnResult result = eval(frame, if_->condition);
    if (result != Fn_OK)
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
    return Fn_OK;
}

FnResult execWhile(Interpreter::StackFrame* frame, const Statement::While_* while_) {
    BaseInterpreter* base = &frame->interp->base;

    for (;;) {
        // Evaluate condition.
        ObjectStack::Boundary localVariableStorageBoundary = base->localVariableStorage.end();
        FnResult result = eval(frame, while_->condition);
        if (result != Fn_OK)
            return result;
        // FIXME: Do implicit conversion to bool
        bool wasTrue = (*base->returnValue.cast<bool>() != 0);
        base->returnValue = {};
        // Delete temporary objects.
        base->localVariableStorage.deleteRange(localVariableStorageBoundary,
                                               base->localVariableStorage.items.end());

        // Either stop, or execute the child block.
        if (!wasTrue)
            return Fn_OK;
        result = execBlock(frame, while_->block);
        if (result != Fn_OK)
            return result;
    }
}

FnResult execAssign(Interpreter::StackFrame* frame, const Statement::Assignment* assign) {
    BaseInterpreter* base = &frame->interp->base;
    ObjectStack::Boundary localVariableStorageBoundary = base->localVariableStorage.end();

    // Evaluate left side.
    AnyObject left;
    if (assign->left->id != Expression::ID::NameLookup) {
        FnResult result = eval(frame, assign->left);
        if (result != Fn_OK)
            return result;
        left = base->returnValue;
        base->returnValue = {};
    }

    // Evaluate right side.
    FnResult result = eval(frame, assign->right);
    if (result != Fn_OK)
        return result;

    // Perform assignment.
    if (assign->left->id == Expression::ID::NameLookup) {
        PLY_ASSERT(!left.data);
        Label name = assign->left->nameLookup()->name;
        if (frame->hooks.assignToLocal(assign->attributes, name))
            return Fn_OK;

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
    return Fn_OK;
}

FnResult execBlock(Interpreter::StackFrame* frame, const StatementBlock* block) {
    BaseInterpreter* base = &frame->interp->base;

    // Execute each statement in this block.
    for (const Statement* statement : block->statements) {
        frame->tokenIdx = statement->tokenIdx;
        switch (statement->id) {
            case Statement::ID::If_: {
                FnResult result = execIf(frame, statement->if_().get());
                if (result != Fn_OK)
                    return result;
                break;
            }
            case Statement::ID::While_: {
                FnResult result = execWhile(frame, statement->while_().get());
                if (result != Fn_OK)
                    return result;
                break;
            }
            case Statement::ID::Assignment: {
                FnResult result = execAssign(frame, statement->assignment().get());
                if (result != Fn_OK)
                    return result;
                break;
            }
            case Statement::ID::Evaluate: {
                ObjectStack::Boundary localVariableStorageBoundary =
                    base->localVariableStorage.end();
                FnResult result = eval(frame, statement->evaluate()->expr);
                if (result != Fn_OK)
                    return result;
                bool hookResult = frame->hooks.onEvaluate(statement->evaluate()->attributes);
                base->returnValue = {};
                // Delete temporary objects.
                base->localVariableStorage.deleteRange(localVariableStorageBoundary,
                                                       base->localVariableStorage.items.end());
                if (!hookResult)
                    return Fn_Error;
                break;
            }
            case Statement::ID::Return_: {
                FnResult result = eval(frame, statement->return_()->expr);
                if (result != Fn_OK)
                    return result;
                return Fn_Return;
            }
            case Statement::ID::CustomBlock: {
                const Statement::CustomBlock* cb = statement->customBlock().get();
                FnResult result = frame->hooks.doCustomBlock(cb);
                if (result != Fn_OK)
                    return result;
                break;
            }
            default: {
                PLY_ASSERT(0);
                return Fn_Error;
            }
        }
    }

    return Fn_OK;
}

FnResult execFunction(Interpreter::StackFrame* frame, const StatementBlock* block) {
    BaseInterpreter* base = &frame->interp->base;
    PLY_SET_IN_SCOPE(frame->interp->currentFrame, frame);
    ObjectStack::Boundary endOfPreviousFrameStorage = base->localVariableStorage.end();

    // Execute function body.
    FnResult result = execBlock(frame, block);
    if (result == Fn_Return)
        result = Fn_OK;

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

} // namespace biscuit
} // namespace ply

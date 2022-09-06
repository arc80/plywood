/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-crowbar/Core.h>
#include <ply-crowbar/Interpreter.h>

namespace ply {
namespace crowbar {

HiddenArgFunctor<HybridString()> makeFunctionDesc(const Statement::FunctionDefinition* fnDef) {
    return {[](const Statement::FunctionDefinition* fnDef) -> HybridString {
                return String::format("function '{}'", g_labelStorage.view(fnDef->name));
            },
            fnDef};
}

void Interpreter::error(StringView message) {
    this->outs->format("error: {}\n", message);
    bool first = true;
    for (Interpreter::StackFrame* frame = this->currentFrame; frame; frame = frame->prevFrame) {
        ExpandedToken expToken = frame->tkr->expandToken(frame->tokenIdx);
        this->outs->format("{} {} {}\n",
                           frame->tkr->fileLocationMap.formatFileLocation(expToken.fileOffset),
                           first ? "in" : "called from", frame->desc());
        first = false;
    }
}

AnyObject MapNamespace::find(Label identifier) const {
    auto cursor = this->map.find(identifier);
    if (cursor.wasFound())
        return cursor->obj;
    return {};
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
    Interpreter* interp = frame->interp;

    MemOutStream mout;
    for (const Expression::InterpolatedString::Piece& piece : stringExp->pieces) {
        mout << piece.literal;
        if (piece.embed) {
            MethodResult result = eval(frame, piece.embed);
            if (result != MethodResult::OK)
                return result;
            write(mout, interp->returnValue);
            interp->returnValue = {};
        }
    }

    // Return string with all substitutions performed.
    AnyObject* stringObj = interp->localVariableStorage.appendObject(getTypeDescriptor<String>());
    *stringObj->cast<String>() = mout.moveToString();
    interp->returnValue = *stringObj;
    return MethodResult::OK;
}

PLY_INLINE bool isReturnValueOnTopOfStack(Interpreter* interp) {
    return !interp->localVariableStorage.items.isEmpty() &&
           (interp->localVariableStorage.items.tail() == interp->returnValue);
}

MethodResult evalPropertyLookup(Interpreter::StackFrame* frame,
                                const Expression::PropertyLookup* propLookup) {
    Interpreter* interp = frame->interp;

    // Evaluate left side.
    MethodResult result = eval(frame, propLookup->obj);
    if (result != MethodResult::OK)
        return result;
    AnyObject obj = interp->returnValue;
    interp->returnValue = {};

    // Perform property lookup.
    // FIXME: This should accept interned strings.
    return obj.type->methods.propertyLookup(interp, obj,
                                            g_labelStorage.view(propLookup->propertyName));
}

MethodResult evalBinaryOp(Interpreter::StackFrame* frame, const Expression::BinaryOp* binaryOp) {
    Interpreter* interp = frame->interp;

    // Evaluate left side.
    MethodResult result = eval(frame, binaryOp->left);
    if (result != MethodResult::OK)
        return result;
    AnyObject left = interp->returnValue;
    interp->returnValue = {};

    // Evaluate right side.
    result = eval(frame, binaryOp->right);
    if (result != MethodResult::OK)
        return result;
    AnyObject right = interp->returnValue;
    interp->returnValue = {};

    // Invoke operator.
    return left.type->methods.binaryOp(interp, binaryOp->op, left, right);
}

MethodResult evalUnaryOp(Interpreter::StackFrame* frame, const Expression::UnaryOp* unaryOp) {
    Interpreter* interp = frame->interp;

    // Evaluate subexpression.
    MethodResult result = eval(frame, unaryOp->expr);
    if (result != MethodResult::OK)
        return result;
    AnyObject obj = interp->returnValue;
    interp->returnValue = {};

    // Invoke operator.
    return obj.type->methods.unaryOp(interp, unaryOp->op, obj);
}

MethodResult evalCall(Interpreter::StackFrame* frame, const Expression::Call* call) {
    Interpreter* interp = frame->interp;

    // Evaluate callable.
    MethodResult result = eval(frame, call->callable);
    if (result != MethodResult::OK)
        return result;
    AnyObject callee = interp->returnValue;
    interp->returnValue = {};

    // Evaluate arguments.
    Array<AnyObject> args;
    for (const Expression* argExpr : call->args) {
        result = eval(frame, argExpr);
        if (result != MethodResult::OK)
            return result;

        AnyObject* arg;
        if (!isReturnValueOnTopOfStack(interp)) {
            // The return value is not a temporary object, and the interpreter is currently designed
            // to pass all arguments "by value", like in C, so we should make a copy here. (In the
            // future, we could extend the interpreter to support passing "by reference" as well,
            // like in C++. In the meantime, scripts can achieve the same thing by passing
            // pointers.)
            arg = interp->localVariableStorage.appendObject(interp->returnValue.type);
            arg->copy(interp->returnValue);
        } else {
            arg = &interp->localVariableStorage.items.tail();
        }
        args.append(*arg);
        interp->returnValue = {};
    }

    // Invoke function using provided arguments.
    if (callee.is<Statement::FunctionDefinition>()) {
        // It's a Crowbar function.
        // FIXME: Move this to MethodTable.
        const auto* functionDef = callee.cast<Statement::FunctionDefinition>();
        PLY_ASSERT(args.numItems() == functionDef->parameterNames.numItems());

        // Set up a new stack frame.
        Interpreter::StackFrame newFrame;
        newFrame.interp = interp;
        newFrame.desc = makeFunctionDesc(functionDef);
        newFrame.tkr = functionDef->tkr;
        newFrame.prevFrame = frame;
        for (u32 argIndex : range(args.numItems())) {
            newFrame.localVariableTable.insertOrFind(functionDef->parameterNames[argIndex])->obj =
                args[argIndex];
        }

        // Execute function body and clean up stack frame.
        return execFunction(&newFrame, functionDef->body);
    } else {
        // Call through MethodTable.
        return callee.type->methods.call(interp, callee, args);
    }
}

AnyObject lookupName(Interpreter::StackFrame* frame, Label name) {
    Interpreter* interp = frame->interp;

    auto cursor = frame->localVariableTable.find(name);
    if (cursor.wasFound())
        return cursor->obj;

    for (s32 i = interp->outerNameSpaces.numItems() - 1; i >= 0; i--) {
        AnyObject obj = interp->outerNameSpaces[i]->find(name);
        if (obj.data)
            return obj;
    }

    return {};
}

MethodResult eval(Interpreter::StackFrame* frame, const Expression* expr) {
    Interpreter* interp = frame->interp;
    PLY_SET_IN_SCOPE(frame->tokenIdx, expr->tokenIdx);

    switch (expr->id) {
        case Expression::ID::NameLookup: {
            interp->returnValue = lookupName(frame, expr->nameLookup()->name);
            if (!interp->returnValue.data) {
                interp->error(String::format("cannot resolve identifier '{}'",
                                             g_labelStorage.view(expr->nameLookup()->name)));
                return MethodResult::Error;
            }
            return MethodResult::OK;
        }

        case Expression::ID::IntegerLiteral: {
            interp->returnValue = AnyObject::bind(&expr->integerLiteral()->value);
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
    Interpreter* interp = frame->interp;

    // Evaluate condition.
    ObjectStack::Boundary localVariableStorageBoundary = interp->localVariableStorage.end();
    MethodResult result = eval(frame, if_->condition);
    if (result != MethodResult::OK)
        return result;
    // FIXME: Do implicit conversion to bool
    bool wasTrue = (*interp->returnValue.cast<bool>() != 0);
    interp->returnValue = {};
    // Delete temporary objects.
    interp->localVariableStorage.deleteRange(localVariableStorageBoundary,
                                             interp->localVariableStorage.items.end());

    // Execute the appropriate child block (if any).
    PLY_ASSERT(if_->trueBlock);
    const StatementBlock* block = (wasTrue ? if_->trueBlock : if_->falseBlock);
    if (block)
        return execBlock(frame, block);
    return MethodResult::OK;
}

MethodResult execWhile(Interpreter::StackFrame* frame, const Statement::While_* while_) {
    Interpreter* interp = frame->interp;

    for (;;) {
        // Evaluate condition.
        ObjectStack::Boundary localVariableStorageBoundary = interp->localVariableStorage.end();
        MethodResult result = eval(frame, while_->condition);
        if (result != MethodResult::OK)
            return result;
        // FIXME: Do implicit conversion to bool
        bool wasTrue = (*interp->returnValue.cast<bool>() != 0);
        interp->returnValue = {};
        // Delete temporary objects.
        interp->localVariableStorage.deleteRange(localVariableStorageBoundary,
                                                 interp->localVariableStorage.items.end());

        // Either stop, or execute the child block.
        if (!wasTrue)
            return MethodResult::OK;
        result = execBlock(frame, while_->block);
        if (result != MethodResult::OK)
            return result;
    }
}

MethodResult execAssign(Interpreter::StackFrame* frame, const Statement::Assignment* assign) {
    Interpreter* interp = frame->interp;
    ObjectStack::Boundary localVariableStorageBoundary = interp->localVariableStorage.end();

    // Evaluate left side.
    AnyObject left;
    if (assign->left->id != Expression::ID::NameLookup) {
        MethodResult result = eval(frame, assign->left);
        if (result != MethodResult::OK)
            return result;
        left = interp->returnValue;
        interp->returnValue = {};
    }

    // Evaluate right side.
    MethodResult result = eval(frame, assign->right);
    if (result != MethodResult::OK)
        return result;

    // Perform assignment.
    if (assign->left->id == Expression::ID::NameLookup) {
        PLY_ASSERT(!left.data);
        Label name = assign->left->nameLookup()->name;
        if (interp->hooks->handleLocalAssignment(name))
            return MethodResult::OK;

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
    return MethodResult::OK;
}

MethodResult execBlock(Interpreter::StackFrame* frame, const StatementBlock* block) {
    Interpreter* interp = frame->interp;

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
                    interp->localVariableStorage.end();
                MethodResult result = eval(frame, statement->evaluate()->expr);
                if (result != MethodResult::OK)
                    return result;
                if (interp->hooks) {
                    interp->hooks->onEvaluate(statement->evaluate()->traits);
                }
                interp->returnValue = {};
                // Delete temporary objects.
                interp->localVariableStorage.deleteRange(localVariableStorageBoundary,
                                                         interp->localVariableStorage.items.end());
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
                const Statement::CustomBlock* prevBlock = interp->currentFrame->customBlock;
                if (interp->hooks) {
                    interp->hooks->enterCustomBlock(cb);
                }
                interp->currentFrame->customBlock = cb;
                MethodResult result = execBlock(frame, cb->body);
                if (interp->hooks) {
                    interp->hooks->exitCustomBlock(cb);
                }
                interp->currentFrame->customBlock = prevBlock;
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
    Interpreter* interp = frame->interp;
    PLY_SET_IN_SCOPE(interp->currentFrame, frame);
    ObjectStack::Boundary endOfPreviousFrameStorage = interp->localVariableStorage.end();

    // Execute function body.
    MethodResult result = execBlock(frame, block);
    if (result == MethodResult::Return)
        result = MethodResult::OK;

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

    return result;
}

} // namespace crowbar
} // namespace ply

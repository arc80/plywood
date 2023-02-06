/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-biscuit/Core.h>
#include <ply-biscuit/Interpreter.h>
#include <ply-reflect/methods/BoundMethod.h>

namespace ply {
namespace biscuit {

void log_error_with_stack(OutStream& out, const Interpreter* interp,
                          StringView message) {
    Interpreter::StackFrame* frame = interp->current_frame;
    ExpandedToken exp_token = frame->tkr->expand_token(frame->token_idx);
    out.format(
        "{} error: {}\n",
        frame->tkr->file_location_map.format_file_location(exp_token.file_offset),
        message);
    for (;;) {
        frame = frame->prev_frame;
        if (!frame)
            break;
        ExpandedToken exp_token = frame->tkr->expand_token(frame->token_idx);
        out.format(
            "{}: called from {}\n",
            frame->tkr->file_location_map.format_file_location(exp_token.file_offset),
            frame->desc());
    }
}

AnyObject find(const Map<Label, AnyObject>& map, Label identifier) {
    const AnyObject* value = map.find(identifier);
    return value ? *value : AnyObject{};
}

FnResult exec_function(Interpreter::StackFrame* frame, const StatementBlock* block);

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

FnResult eval_string(Interpreter::StackFrame* frame,
                     const Expression::InterpolatedString* string_exp) {
    BaseInterpreter* base = &frame->interp->base;

    MemOutStream mout;
    for (const Expression::InterpolatedString::Piece& piece : string_exp->pieces) {
        mout << piece.literal;
        if (piece.embed) {
            FnResult result = eval(frame, piece.embed);
            if (result != Fn_OK)
                return result;
            write(mout, base->return_value);
            base->return_value = {};
        }
    }

    // Return string with all substitutions performed.
    AnyObject* string_obj =
        base->local_variable_storage.append_object(get_type_descriptor<String>());
    *string_obj->cast<String>() = mout.move_to_string();
    base->return_value = *string_obj;
    return Fn_OK;
}

PLY_INLINE bool is_return_value_on_top_of_stack(BaseInterpreter* base) {
    return !base->local_variable_storage.items.is_empty() &&
           (base->local_variable_storage.items.tail() == base->return_value);
}

FnResult eval_property_lookup(Interpreter::StackFrame* frame,
                              const Expression::PropertyLookup* prop_lookup) {
    BaseInterpreter* base = &frame->interp->base;

    // Evaluate left side.
    FnResult result = eval(frame, prop_lookup->obj);
    if (result != Fn_OK)
        return result;
    AnyObject obj = base->return_value;
    base->return_value = {};

    // Perform property lookup.
    // FIXME: This should accept interned strings.
    return obj.type->methods.property_lookup(
        base, obj, g_labelStorage.view(prop_lookup->property_name));
}

FnResult eval_binary_op(Interpreter::StackFrame* frame,
                        const Expression::BinaryOp* binary_op) {
    BaseInterpreter* base = &frame->interp->base;

    // Evaluate left side.
    FnResult result = eval(frame, binary_op->left);
    if (result != Fn_OK)
        return result;
    AnyObject left = base->return_value;
    base->return_value = {};

    // Evaluate right side.
    result = eval(frame, binary_op->right);
    if (result != Fn_OK)
        return result;
    AnyObject right = base->return_value;
    base->return_value = {};

    // Invoke operator.
    return left.type->methods.binary_op(base, binary_op->op, left, right);
}

FnResult eval_unary_op(Interpreter::StackFrame* frame,
                       const Expression::UnaryOp* unary_op) {
    BaseInterpreter* base = &frame->interp->base;

    // Evaluate subexpression.
    FnResult result = eval(frame, unary_op->expr);
    if (result != Fn_OK)
        return result;
    AnyObject obj = base->return_value;
    base->return_value = {};

    // Invoke operator.
    return obj.type->methods.unary_op(base, unary_op->op, obj);
}

FnResult eval_call(Interpreter::StackFrame* frame, const Expression::Call* call) {
    BaseInterpreter* base = &frame->interp->base;

    // Evaluate callable.
    FnResult result = eval(frame, call->callable);
    if (result != Fn_OK)
        return result;
    AnyObject callee = base->return_value;
    base->return_value = {};

    // Evaluate arguments.
    Array<AnyObject> args;
    for (const Expression* arg_expr : call->args) {
        result = eval(frame, arg_expr);
        if (result != Fn_OK)
            return result;

        AnyObject* arg;
        if (!is_return_value_on_top_of_stack(base)) {
            // The return value is not a temporary object, and the interpreter is
            // currently designed to pass all arguments "by value", like in C, so we
            // should make a copy here. (In the future, we could extend the interpreter
            // to support passing "by reference" as well, like in C++. In the meantime,
            // scripts can achieve the same thing by passing pointers.)
            arg = base->local_variable_storage.append_object(base->return_value.type);
            arg->copy(base->return_value);
        } else {
            arg = &base->local_variable_storage.items.tail();
        }
        args.append(*arg);
        base->return_value = {};
    }

    // Handle bound methods.
    AnyObject self;
    if (BoundMethod* bm = callee.safe_cast<BoundMethod>()) {
        self = bm->target;
        callee = bm->func;
    }

    // Invoke the callee with the provided arguments.
    if (const auto* function_def = callee.safe_cast<Statement::FunctionDefinition>()) {
        // Function is implemented in script.
        PLY_ASSERT(!self); // Not possible to define classes in script yet
        PLY_ASSERT(args.num_items() == function_def->parameter_names.num_items());

        // Set up a new stack frame.
        Interpreter::StackFrame new_frame;
        new_frame.interp = frame->interp;
        new_frame.desc = [function_def]() -> HybridString {
            return String::format("function '{}'",
                                  g_labelStorage.view(function_def->name));
        };
        new_frame.tkr = function_def->tkr;
        new_frame.prev_frame = frame;
        for (u32 arg_index = 0; arg_index < args.num_items(); arg_index++) {
            new_frame.local_variable_table.assign(
                function_def->parameter_names[arg_index], args[arg_index]);
        }

        // Execute function body and clean up stack frame.
        return exec_function(&new_frame, function_def->body);
    } else if (NativeFunction* func = callee.safe_cast<NativeFunction>()) {
        FnParams params;
        params.base = base;
        params.self = self;
        params.args = args;
        return func(params);
    } else if (BoundNativeMethod* bnm = callee.safe_cast<BoundNativeMethod>()) {
        FnParams params;
        params.base = base;
        params.self = self;
        params.args = args;
        return bnm->func(bnm->self, params);
    }

    // Object is not callable
    base->error(
        String::format("cannot call '{}' as a function", callee.type->get_name()));
    return Fn_Error;
}

AnyObject lookup_name(Interpreter::StackFrame* frame, Label name) {
    // Check local variables first.
    AnyObject* value = frame->local_variable_table.find(name);
    if (value)
        return *value;

    // Then client.
    return frame->interp->resolve_name(name);
}

FnResult eval(Interpreter::StackFrame* frame, const Expression* expr) {
    BaseInterpreter* base = &frame->interp->base;
    PLY_SET_IN_SCOPE(frame->token_idx, expr->token_idx);

    switch (expr->id) {
        case Expression::ID::NameLookup: {
            base->return_value = lookup_name(frame, expr->name_lookup()->name);
            if (!base->return_value.data) {
                base->error(
                    String::format("cannot resolve identifier '{}'",
                                   g_labelStorage.view(expr->name_lookup()->name)));
                return Fn_Error;
            }
            return Fn_OK;
        }

        case Expression::ID::IntegerLiteral: {
            base->return_value = AnyObject::bind(&expr->integer_literal()->value);
            return Fn_OK;
        }

        case Expression::ID::InterpolatedString: {
            return eval_string(frame, expr->interpolated_string().get());
        }

        case Expression::ID::PropertyLookup: {
            return eval_property_lookup(frame, expr->property_lookup().get());
        }

        case Expression::ID::BinaryOp: {
            return eval_binary_op(frame, expr->binary_op().get());
        }

        case Expression::ID::UnaryOp: {
            return eval_unary_op(frame, expr->unary_op().get());
        }

        case Expression::ID::Call: {
            return eval_call(frame, expr->call().get());
        }

        default: {
            PLY_ASSERT(0);
            return Fn_Error;
        }
    }
}

FnResult exec_if(Interpreter::StackFrame* frame, const Statement::If_* if_) {
    BaseInterpreter* base = &frame->interp->base;

    // Evaluate condition.
    ObjectStack::Boundary local_variable_storage_boundary =
        base->local_variable_storage.end();
    FnResult result = eval(frame, if_->condition);
    if (result != Fn_OK)
        return result;
    // FIXME: Do implicit conversion to bool
    bool was_true = (*base->return_value.cast<bool>() != 0);
    base->return_value = {};
    // Delete temporary objects.
    base->local_variable_storage.delete_range(local_variable_storage_boundary,
                                              base->local_variable_storage.items.end());

    // Execute the appropriate child block (if any).
    PLY_ASSERT(if_->true_block);
    const StatementBlock* block = (was_true ? if_->true_block : if_->false_block);
    if (block)
        return exec_block(frame, block);
    return Fn_OK;
}

FnResult exec_while(Interpreter::StackFrame* frame, const Statement::While_* while_) {
    BaseInterpreter* base = &frame->interp->base;

    for (;;) {
        // Evaluate condition.
        ObjectStack::Boundary local_variable_storage_boundary =
            base->local_variable_storage.end();
        FnResult result = eval(frame, while_->condition);
        if (result != Fn_OK)
            return result;
        // FIXME: Do implicit conversion to bool
        bool was_true = (*base->return_value.cast<bool>() != 0);
        base->return_value = {};
        // Delete temporary objects.
        base->local_variable_storage.delete_range(
            local_variable_storage_boundary, base->local_variable_storage.items.end());

        // Either stop, or execute the child block.
        if (!was_true)
            return Fn_OK;
        result = exec_block(frame, while_->block);
        if (result != Fn_OK)
            return result;
    }
}

FnResult exec_assign(Interpreter::StackFrame* frame,
                     const Statement::Assignment* assign) {
    BaseInterpreter* base = &frame->interp->base;
    ObjectStack::Boundary local_variable_storage_boundary =
        base->local_variable_storage.end();

    // Evaluate left side.
    AnyObject left;
    if (assign->left->id != Expression::ID::NameLookup) {
        FnResult result = eval(frame, assign->left);
        if (result != Fn_OK)
            return result;
        left = base->return_value;
        base->return_value = {};
    }

    // Evaluate right side.
    FnResult result = eval(frame, assign->right);
    if (result != Fn_OK)
        return result;

    // Perform assignment.
    if (assign->left->id == Expression::ID::NameLookup) {
        PLY_ASSERT(!left.data);
        Label name = assign->left->name_lookup()->name;
        if (frame->hooks.assign_to_local(assign->attributes, name))
            return Fn_OK;

        bool was_found = false;
        AnyObject* value = frame->local_variable_table.insert_or_find(name, &was_found);
        if (!was_found) {
            if (is_return_value_on_top_of_stack(base)) {
                WeakSequenceRef<AnyObject> delete_to =
                    base->local_variable_storage.items.end();
                --delete_to;
                // Delete temporary objects except for the return value.
                base->local_variable_storage.delete_range(
                    local_variable_storage_boundary, delete_to);
                // We've just created a new local variable.
                *value = base->local_variable_storage.items.tail();
            } else {
                // Delete temporary objects.
                base->local_variable_storage.delete_range(
                    local_variable_storage_boundary,
                    base->local_variable_storage.items.end());
                // Allocate storage for new local variable.
                AnyObject* dest =
                    base->local_variable_storage.append_object(base->return_value.type);
                dest->move(base->return_value);
                *value = *dest;
            }
        } else {
            // Move result to existing local variable.
            value->move(base->return_value);
            // Delete temporary objects.
            base->local_variable_storage.delete_range(
                local_variable_storage_boundary,
                base->local_variable_storage.items.end());
        }
        base->return_value = {};
    } else {
        left.move(base->return_value);
        base->return_value = {};
        // Delete temporary objects.
        base->local_variable_storage.delete_range(
            local_variable_storage_boundary, base->local_variable_storage.items.end());
    }
    return Fn_OK;
}

FnResult exec_block(Interpreter::StackFrame* frame, const StatementBlock* block) {
    BaseInterpreter* base = &frame->interp->base;

    // Execute each statement in this block.
    for (const Statement* statement : block->statements) {
        frame->token_idx = statement->token_idx;
        switch (statement->id) {
            case Statement::ID::If_: {
                FnResult result = exec_if(frame, statement->if_().get());
                if (result != Fn_OK)
                    return result;
                break;
            }
            case Statement::ID::While_: {
                FnResult result = exec_while(frame, statement->while_().get());
                if (result != Fn_OK)
                    return result;
                break;
            }
            case Statement::ID::Assignment: {
                FnResult result = exec_assign(frame, statement->assignment().get());
                if (result != Fn_OK)
                    return result;
                break;
            }
            case Statement::ID::Evaluate: {
                ObjectStack::Boundary local_variable_storage_boundary =
                    base->local_variable_storage.end();
                FnResult result = eval(frame, statement->evaluate()->expr);
                if (result != Fn_OK)
                    return result;
                bool hook_result =
                    frame->hooks.on_evaluate(statement->evaluate()->attributes);
                base->return_value = {};
                // Delete temporary objects.
                base->local_variable_storage.delete_range(
                    local_variable_storage_boundary,
                    base->local_variable_storage.items.end());
                if (!hook_result)
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
                const Statement::CustomBlock* cb = statement->custom_block().get();
                FnResult result = frame->hooks.do_custom_block(cb);
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

FnResult exec_function(Interpreter::StackFrame* frame, const StatementBlock* block) {
    BaseInterpreter* base = &frame->interp->base;
    PLY_SET_IN_SCOPE(frame->interp->current_frame, frame);
    ObjectStack::Boundary end_of_previous_frame_storage =
        base->local_variable_storage.end();

    // Execute function body.
    FnResult result = exec_block(frame, block);
    if (result == Fn_Return)
        result = Fn_OK;

    // Destroy all local variables in this stack frame.
    WeakSequenceRef<AnyObject> delete_to = base->local_variable_storage.items.end();
    bool fixup_return_value =
        (end_of_previous_frame_storage != base->local_variable_storage.end()) &&
        (base->local_variable_storage.items.tail() == base->return_value);
    if (fixup_return_value) {
        --delete_to;
    }
    base->local_variable_storage.delete_range(end_of_previous_frame_storage, delete_to);
    if (fixup_return_value) {
        PLY_ASSERT(base->local_variable_storage.items.tail().type ==
                   base->return_value.type);
        base->return_value.data = base->local_variable_storage.items.tail().data;
    }

    return result;
}

} // namespace biscuit
} // namespace ply

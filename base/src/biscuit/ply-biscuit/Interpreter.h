/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once
#include <ply-biscuit/Core.h>
#include <ply-reflect/methods/BaseInterpreter.h>
#include <ply-biscuit/ParseTree.h>

namespace ply {
namespace biscuit {

struct Tokenizer;

struct Interpreter {
    struct Hooks {
        Func<FnResult(const Statement::CustomBlock* custom_block)> do_custom_block;
        Func<bool(const AnyObject& attributes)> on_evaluate = [](const AnyObject&) {
            return true;
        };
        Func<bool(const AnyObject& attributes, Label label)> assign_to_local;
    };

    struct StackFrame {
        Interpreter* interp = nullptr;
        Func<HybridString()> desc;
        Map<Label, AnyObject> local_variable_table;
        Tokenizer* tkr = nullptr;
        const Statement::CustomBlock* custom_block = nullptr;
        u32 token_idx = 0;
        StackFrame* prev_frame = nullptr;
        Hooks hooks;
    };

    BaseInterpreter base;
    Func<AnyObject(Label identifier)> resolve_name;
    StackFrame* current_frame = nullptr;
};

void log_error_with_stack(OutStream& out, const Interpreter* interp,
                          StringView message);
FnResult exec_function(Interpreter::StackFrame* frame, const StatementBlock* block);
FnResult exec_block(Interpreter::StackFrame* frame, const StatementBlock* block);
FnResult eval(Interpreter::StackFrame* frame, const Expression* expr);

} // namespace biscuit
} // namespace ply

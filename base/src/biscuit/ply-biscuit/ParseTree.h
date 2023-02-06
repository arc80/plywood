﻿/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once
#include <ply-biscuit/Core.h>
#include <ply-biscuit/Tokenizer.h>
#include <ply-reflect/AnyOwnedObject.h>

namespace ply {
namespace biscuit {

struct StatementBlock;

struct Expression {
    u32 token_idx = 0;

    // ply make switch
    struct NameLookup {
        Label name;
    };
    struct IntegerLiteral {
        u32 value = 0;
    };
    struct InterpolatedString {
        struct Piece {
            StringView literal; // Uses Label storage
            Owned<Expression> embed;
        };
        Array<Piece> pieces;
    };
    struct PropertyLookup {
        Owned<Expression> obj;
        Label property_name;
    };
    struct BinaryOp {
        MethodTable::BinaryOp op;
        Owned<Expression> left;
        Owned<Expression> right;
    };
    struct UnaryOp {
        MethodTable::UnaryOp op;
        Owned<Expression> expr;
    };
    struct Call {
        Owned<Expression> callable;
        Array<Owned<Expression>> args;
    };
#include "codegen/switch-ply-biscuit-Expression.inl" //@@ply
};

struct Statement {
    u32 token_idx = 0; // FIXME: Delete this member because it's not actually useful?
    u32 file_offset = 0;

    // ply make switch
    struct If_ {
        Owned<Expression> condition;
        Owned<StatementBlock> true_block;
        Owned<StatementBlock> false_block;
    };
    struct While_ {
        Owned<Expression> condition;
        Owned<StatementBlock> block;
    };
    struct Assignment {
        Owned<Expression> left;
        Owned<Expression> right;
        AnyOwnedObject attributes;
    };
    struct Evaluate {
        Owned<Expression> expr;
        AnyOwnedObject attributes;
    };
    struct Return_ {
        Owned<Expression> expr;
    };
    struct FunctionDefinition {
        PLY_REFLECT()
        // ply reflect off

        Label name;
        Tokenizer* tkr = nullptr;
        Array<Label> parameter_names;
        Owned<StatementBlock> body;
    };
    struct CustomBlock {
        Label type;
        Label name;
        Owned<Expression> expr;
        Owned<StatementBlock> body;
    };
#include "codegen/switch-ply-biscuit-Statement.inl" //@@ply
};

struct StatementBlock {
    Array<Owned<Statement>> statements;
};

} // namespace biscuit
} // namespace ply

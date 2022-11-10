/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-crowbar/Core.h>
#include <ply-crowbar/Tokenizer.h>

namespace ply {
namespace crowbar {

struct StatementBlock;

struct Expression {
    u32 tokenIdx = 0;

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
        Label propertyName;
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
#include "codegen/switch-ply-crowbar-Expression.inl" //@@ply
};

struct Statement {
    u32 tokenIdx = 0; // FIXME: Delete this member because it's not actually useful?
    u32 fileOffset = 0;

    // ply make switch
    struct If_ {
        Owned<Expression> condition;
        Owned<StatementBlock> trueBlock;
        Owned<StatementBlock> falseBlock;
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
        Array<Label> parameterNames;
        Owned<StatementBlock> body;
    };
    struct CustomBlock {
        Label type;
        Label name;
        Owned<Expression> expr;
        Owned<StatementBlock> body;
    };
#include "codegen/switch-ply-crowbar-Statement.inl" //@@ply
};

struct StatementBlock {
    Array<Owned<Statement>> statements;
};

} // namespace crowbar
} // namespace ply

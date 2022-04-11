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

struct FunctionDefinition {
    u32 name; // Interned string.
    Array<u32> parameterNames;
    Owned<StatementBlock> body;
};

struct Expression {
    // ply make switch
    struct NameLookup {
        u32 name; // Interned string.
    };
    struct IntegerLiteral {
        u32 value;
    };
    struct PropertyLookup {
        Owned<Expression> obj;
        u32 propertyName; // Interned string.
    };
    struct BinaryOp {
        MethodTable::BinaryOp op;
        Owned<Expression> left;
        Owned<Expression> right;
    };
    struct Call {
        Owned<Expression> callable;
        Array<Owned<Expression>> args;
    };
    struct Custom {
        void* data = nullptr;
    };
#include "codegen/switch-ply-crowbar-Expression.inl" //@@ply
};

struct Statement {
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
    };
    struct Evaluate {
        Owned<Expression> expr;
    };
    struct Return_ {
        Owned<Expression> expr;
    };
#include "codegen/switch-ply-crowbar-Statement.inl" //@@ply
};

struct StatementBlock {
    Array<Owned<Statement>> statements;
};

struct File {
    Array<Owned<FunctionDefinition>> functions;
};

} // namespace crowbar

PLY_DECLARE_TYPE_DESCRIPTOR(crowbar::FunctionDefinition)

} // namespace ply

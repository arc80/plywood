/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once
#include <ply-reflect/Core.h>

#if PLY_WITH_METHOD_TABLES

namespace ply {

struct BaseInterpreter;
struct AnyObject;

enum FnResult {
    Fn_OK,
    Fn_Return,
    Fn_Error,
};

struct MethodTable {
    enum class UnaryOp {
        Invalid = 0,

        Negate,
        LogicalNot,
        BitComplement,

        // Container protocol:
        ContainerStart,
        IsEmpty = ContainerStart,
        NumItems,

        // List protocol:
        ListStart,
        Head = ListStart,
        Tail,
        Dereference,
        Next,
        Prev,

        Count
    };

    enum class BinaryOp {
        Invalid = 0,
        // C operator precedence level 3
        Multiply,
        Divide,
        Modulo,
        // C operator precedence level 4
        Add,
        Subtract,
        // C operator precedence level 6
        LessThan,
        LessThanOrEqual,
        GreaterThan,
        GreaterThanOrEqual,
        // C operator precedence level 7
        DoubleEqual,
        // C operator precedence level 11
        LogicalAnd,
        // C operator precedence level 12
        LogicalOr,

        Count
    };

    // Static member functions
    static StringView unaryOpToString(UnaryOp op);
    static StringView binaryOpToString(BinaryOp op);
    static FnResult unsupportedUnaryOp(BaseInterpreter* interp, UnaryOp op,
                                           const AnyObject& obj);
    static FnResult unsupportedBinaryOp(BaseInterpreter* interp, MethodTable::BinaryOp op,
                                            const AnyObject& first, const AnyObject& second);
    static FnResult unsupportedPropertyLookup(BaseInterpreter* interp, const AnyObject& obj,
                                                  StringView propertyName);
    static FnResult unsupportedSubscript(BaseInterpreter* interp, const AnyObject& obj,
                                             u32 index);
    static FnResult unsupportedPrint(BaseInterpreter* interp, const AnyObject& obj,
                                         StringView formatSpec);

    // Member variables
    FnResult (*unaryOp)(BaseInterpreter* interp, UnaryOp op, const AnyObject& obj) = nullptr;
    FnResult (*binaryOp)(BaseInterpreter* interp, BinaryOp op, const AnyObject& first,
                             const AnyObject& second) = nullptr;
    FnResult (*propertyLookup)(BaseInterpreter* interp, const AnyObject& obj,
                                   StringView propertyName) = nullptr;
    FnResult (*subscript)(BaseInterpreter* interp, const AnyObject& obj, u32 index) = nullptr;
    FnResult (*print)(BaseInterpreter* interp, const AnyObject& obj,
                          StringView formatSpec) = nullptr;

    // Constructor
    MethodTable();
};

} // namespace ply

#endif // PLY_WITH_METHOD_TABLES

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
    static StringView unary_op_to_string(UnaryOp op);
    static StringView binary_op_to_string(BinaryOp op);
    static FnResult unsupported_unary_op(BaseInterpreter* interp, UnaryOp op,
                                         const AnyObject& obj);
    static FnResult unsupported_binary_op(BaseInterpreter* interp,
                                          MethodTable::BinaryOp op,
                                          const AnyObject& first,
                                          const AnyObject& second);
    static FnResult unsupported_property_lookup(BaseInterpreter* interp,
                                                const AnyObject& obj,
                                                StringView property_name);
    static FnResult unsupported_subscript(BaseInterpreter* interp, const AnyObject& obj,
                                          u32 index);
    static FnResult unsupported_print(BaseInterpreter* interp, const AnyObject& obj,
                                      StringView format_spec);

    // Member variables
    FnResult (*unary_op)(BaseInterpreter* interp, UnaryOp op,
                         const AnyObject& obj) = nullptr;
    FnResult (*binary_op)(BaseInterpreter* interp, BinaryOp op, const AnyObject& first,
                          const AnyObject& second) = nullptr;
    FnResult (*property_lookup)(BaseInterpreter* interp, const AnyObject& obj,
                                StringView property_name) = nullptr;
    FnResult (*subscript)(BaseInterpreter* interp, const AnyObject& obj,
                          u32 index) = nullptr;
    FnResult (*print)(BaseInterpreter* interp, const AnyObject& obj,
                      StringView format_spec) = nullptr;

    // Constructor
    MethodTable();
};

} // namespace ply

#endif // PLY_WITH_METHOD_TABLES

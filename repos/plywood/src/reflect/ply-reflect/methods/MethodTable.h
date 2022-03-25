/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-reflect/Core.h>

#if PLY_WITH_METHOD_TABLES

namespace ply {

struct ObjectStack;
struct AnyObject;
struct OutStream;

struct MethodTable {
    enum class UnaryOp {
        Negate,
        LogicalNot,
        BitComplement,

        // Container protocol:
        IsEmpty,
        NumItems,

        // List protocol:
        Head,
        Tail,
        Dereference,
        Next,
        Prev,
    };

    enum class BinaryOp {
        // C operator precedence level 3
        Multiply = 0,
        Divide,
        Modulo,
        // C operator precedence level 4
        Add,
        Subtract,
    };

    AnyObject (*unaryOp)(ObjectStack* stack, UnaryOp op, const AnyObject& obj) = nullptr;
    AnyObject (*binaryOp)(ObjectStack* stack, BinaryOp op, const AnyObject& first,
                     const AnyObject& second) = nullptr;
    AnyObject (*propertyLookup)(ObjectStack* stack, const AnyObject& obj,
                           StringView propertyName) = nullptr;
    AnyObject (*subscript)(ObjectStack* stack, const AnyObject& obj, u32 index) = nullptr;
    void (*print)(ObjectStack* stack, OutStream* outs, StringView formatSpec,
                  const AnyObject& obj) = nullptr;
    AnyObject (*call)(ObjectStack* stack, const AnyObject& obj) = nullptr;

    MethodTable();
};

} // namespace ply

#endif // PLY_WITH_METHOD_TABLES

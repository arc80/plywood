/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-reflect/Core.h>
#include <ply-reflect/methods/MethodTable.h>

#if PLY_WITH_METHOD_TABLES

#include <ply-reflect/methods/BaseInterpreter.h>
#include <ply-reflect/AnyObject.h>

namespace ply {

StringView MethodTable::unary_op_to_string(MethodTable::UnaryOp op) {
    static StringView table[] = {
        "???",
        "-",
        "!",
        "~",
    };
    PLY_STATIC_ASSERT(PLY_STATIC_ARRAY_SIZE(table) ==
                      (u32) MethodTable::UnaryOp::ContainerStart);
    if ((u32) op < (u32) MethodTable::UnaryOp::ContainerStart) {
        return table[(u32) op];
    }
    return "???";
}

StringView MethodTable::binary_op_to_string(MethodTable::BinaryOp op) {
    static StringView table[] = {
        "???", "*", "/", "%", "+", "-", "<", "<=", ">", ">=", "==", "&&", "||",
    };
    PLY_STATIC_ASSERT(PLY_STATIC_ARRAY_SIZE(table) ==
                      (u32) MethodTable::BinaryOp::Count);
    if ((u32) op < (u32) MethodTable::BinaryOp::Count) {
        return table[(u32) op];
    }
    return "???";
}

FnResult MethodTable::unsupported_unary_op(BaseInterpreter* interp, UnaryOp op,
                                           const AnyObject& obj) {
    interp->return_value = {};
    interp->error(String::format("'{}' does not support unary operator '{}'",
                                 obj.type->get_name(),
                                 MethodTable::unary_op_to_string(op)));
    return Fn_Error;
}

FnResult MethodTable::unsupported_binary_op(BaseInterpreter* interp,
                                            MethodTable::BinaryOp op,
                                            const AnyObject& first,
                                            const AnyObject& second) {
    interp->return_value = {};
    interp->error(String::format("'{}' does not support binary operator '{}'",
                                 first.type->get_name(),
                                 MethodTable::binary_op_to_string(op)));
    return Fn_Error;
}

FnResult MethodTable::unsupported_property_lookup(BaseInterpreter* interp,
                                                  const AnyObject& obj,
                                                  StringView property_name) {
    interp->return_value = {};
    interp->error(
        String::format("'{}' does not support property lookup", obj.type->get_name()));
    return Fn_Error;
}

FnResult MethodTable::unsupported_subscript(BaseInterpreter* interp,
                                            const AnyObject& obj, u32 index) {
    interp->return_value = {};
    interp->error(
        String::format("'{}' does not support array indexing", obj.type->get_name()));
    return Fn_Error;
}

FnResult MethodTable::unsupported_print(BaseInterpreter* interp, const AnyObject& obj,
                                        StringView format_spec) {
    interp->return_value = {};
    interp->error(
        String::format("'{}' does not support printing", obj.type->get_name()));
    return Fn_Error;
}

MethodTable::MethodTable()
    : unary_op{unsupported_unary_op}, binary_op{unsupported_binary_op},
      property_lookup{unsupported_property_lookup}, subscript{unsupported_subscript},
      print{unsupported_print} {
}

} // namespace ply

#endif // PLY_WITH_METHOD_TABLES

/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-reflect/Core.h>
#include <ply-reflect/builtin/TypeDescriptor_Bool.h>
#include <ply-reflect/AnyObject.h>

#if PLY_WITH_METHOD_TABLES
#include <ply-reflect/methods/BaseInterpreter.h>
#endif // PLY_WITH_METHOD_TABLES

namespace ply {

#if PLY_WITH_METHOD_TABLES

struct BoolMethodTable {
    static PLY_INLINE MethodTable make() {
        MethodTable methods;
        methods.binary_op = [](BaseInterpreter* interp, MethodTable::BinaryOp op,
                               const AnyObject& first,
                               const AnyObject& second) -> FnResult {
            switch (op) {
                case MethodTable::BinaryOp::DoubleEqual: {
                    interp->return_value =
                        *interp->local_variable_storage.append_object(
                            get_type_descriptor<bool>());
                    *interp->return_value.cast<bool>() =
                        (*first.cast<bool>() == *second.cast<bool>());
                    return Fn_OK;
                }
                case MethodTable::BinaryOp::LogicalAnd: {
                    interp->return_value =
                        *interp->local_variable_storage.append_object(
                            get_type_descriptor<bool>());
                    *interp->return_value.cast<bool>() =
                        (*first.cast<bool>() && *second.cast<bool>());
                    return Fn_OK;
                }
                case MethodTable::BinaryOp::LogicalOr: {
                    interp->return_value =
                        *interp->local_variable_storage.append_object(
                            get_type_descriptor<bool>());
                    *interp->return_value.cast<bool>() =
                        (*first.cast<bool>() || *second.cast<bool>());
                    return Fn_OK;
                }
                default: {
                    return MethodTable::unsupported_binary_op(interp, op, first,
                                                              second);
                }
            }
        };
        methods.unary_op = [](BaseInterpreter* interp, MethodTable::UnaryOp op,
                              const AnyObject& obj) -> FnResult {
            switch (op) {
                case MethodTable::UnaryOp::LogicalNot: {
                    interp->return_value =
                        *interp->local_variable_storage.append_object(
                            get_type_descriptor<bool>());
                    *interp->return_value.cast<bool>() = !*obj.cast<bool>();
                    return Fn_OK;
                }
                default: {
                    return MethodTable::unsupported_unary_op(interp, op, obj);
                }
            }
        };
        return methods;
    }
};

#endif // PLY_WITH_METHOD_TABLES

PLY_DEFINE_TYPE_DESCRIPTOR(bool) {
    static TypeDescriptor type_desc{
        &TypeKey_Bool, (bool*) nullptr,
        NativeBindings::make<bool>() PLY_METHOD_TABLES_ONLY(, BoolMethodTable::make())};
    return &type_desc;
};

} // namespace ply

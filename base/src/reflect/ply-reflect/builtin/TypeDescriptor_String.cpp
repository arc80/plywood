﻿/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-reflect/Core.h>
#include <ply-reflect/builtin/TypeDescriptor_String.h>
#include <ply-reflect/builtin/TypeDescriptor_Arithmetic.h>
#include <ply-reflect/builtin/TypeDescriptor_Bool.h>
#include <ply-reflect/AnyObject.h>

#if PLY_WITH_METHOD_TABLES
#include <ply-reflect/methods/BaseInterpreter.h>
#endif // PLY_WITH_METHOD_TABLES

namespace ply {

#if PLY_WITH_METHOD_TABLES

struct StringMethodTable {
    static MethodTable make() {
        MethodTable methods;
        methods.binary_op = [](BaseInterpreter* interp, MethodTable::BinaryOp op,
                               const AnyObject& first, const AnyObject& second) {
            switch (op) {
                case MethodTable::BinaryOp::Add: {
                    interp->return_value =
                        *interp->local_variable_storage.append_object(
                            get_type_descriptor<String>());
                    *interp->return_value.cast<String>() =
                        *first.cast<String>() + *second.cast<String>();
                    return Fn_OK;
                }
                case MethodTable::BinaryOp::Multiply: {
                    interp->return_value =
                        *interp->local_variable_storage.append_object(
                            get_type_descriptor<String>());
                    *interp->return_value.cast<String>() =
                        *first.cast<String>() * *second.cast<u32>();
                    return Fn_OK;
                }
                case MethodTable::BinaryOp::DoubleEqual: {
                    interp->return_value =
                        *interp->local_variable_storage.append_object(
                            get_type_descriptor<bool>());
                    *interp->return_value.cast<bool>() =
                        (*first.cast<String>() == *second.cast<String>());
                    return Fn_OK;
                }
                default: {
                    return MethodTable::unsupported_binary_op(interp, op, first,
                                                              second);
                }
            }
        };
        return methods;
    }
};

#endif // PLY_WITH_METHOD_TABLES

PLY_DEFINE_TYPE_DESCRIPTOR(String) {
    static TypeDescriptor type_desc{
        &TypeKey_String, (String*) nullptr,
        NativeBindings::make<String>()
            PLY_METHOD_TABLES_ONLY(, StringMethodTable::make())};
    return &type_desc;
}

} // namespace ply

/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-reflect/Core.h>
#include <ply-reflect/builtin/TypeDescriptor_Arithmetic.h>
#include <ply-reflect/builtin/TypeDescriptor_Bool.h>
#include <ply-reflect/AnyObject.h>

#if PLY_WITH_METHOD_TABLES
#include <ply-reflect/methods/BaseInterpreter.h>
#endif // PLY_WITH_METHOD_TABLES

namespace ply {

#if PLY_WITH_METHOD_TABLES

struct ArithmeticMethodTable {
    template <typename T>
    static MethodTable make() {
        MethodTable methods;
        methods.binary_op = [](BaseInterpreter* interp, MethodTable::BinaryOp op,
                               const AnyObject& first,
                               const AnyObject& second) -> FnResult {
            switch (op) {
                case MethodTable::BinaryOp::Multiply: {
                    interp->return_value =
                        *interp->local_variable_storage.append_object(
                            get_type_descriptor<T>());
                    *interp->return_value.cast<T>() =
                        *first.cast<T>() * *second.cast<T>();
                    return Fn_OK;
                }
                case MethodTable::BinaryOp::Divide: {
                    interp->return_value =
                        *interp->local_variable_storage.append_object(
                            get_type_descriptor<T>());
                    *interp->return_value.cast<T>() =
                        *first.cast<T>() / *second.cast<T>();
                    return Fn_OK;
                }
                case MethodTable::BinaryOp::Modulo: {
                    interp->return_value =
                        *interp->local_variable_storage.append_object(
                            get_type_descriptor<T>());
                    *interp->return_value.cast<T>() =
                        *first.cast<T>() % *second.cast<T>();
                    return Fn_OK;
                }
                case MethodTable::BinaryOp::Add: {
                    interp->return_value =
                        *interp->local_variable_storage.append_object(
                            get_type_descriptor<T>());
                    *interp->return_value.cast<T>() =
                        *first.cast<T>() + *second.cast<T>();
                    return Fn_OK;
                }
                case MethodTable::BinaryOp::Subtract: {
                    interp->return_value =
                        *interp->local_variable_storage.append_object(
                            get_type_descriptor<T>());
                    *interp->return_value.cast<T>() =
                        *first.cast<T>() - *second.cast<T>();
                    return Fn_OK;
                }
                case MethodTable::BinaryOp::DoubleEqual: {
                    interp->return_value =
                        *interp->local_variable_storage.append_object(
                            get_type_descriptor<bool>());
                    *interp->return_value.cast<bool>() =
                        (*first.cast<T>() == *second.cast<T>());
                    return Fn_OK;
                }
                case MethodTable::BinaryOp::LessThan: {
                    interp->return_value =
                        *interp->local_variable_storage.append_object(
                            get_type_descriptor<bool>());
                    *interp->return_value.cast<bool>() =
                        (*first.cast<T>() < *second.cast<T>());
                    return Fn_OK;
                }
                case MethodTable::BinaryOp::LessThanOrEqual: {
                    interp->return_value =
                        *interp->local_variable_storage.append_object(
                            get_type_descriptor<bool>());
                    *interp->return_value.cast<bool>() =
                        (*first.cast<T>() <= *second.cast<T>());
                    return Fn_OK;
                }
                case MethodTable::BinaryOp::GreaterThan: {
                    interp->return_value =
                        *interp->local_variable_storage.append_object(
                            get_type_descriptor<bool>());
                    *interp->return_value.cast<bool>() =
                        (*first.cast<T>() > *second.cast<T>());
                    return Fn_OK;
                }
                case MethodTable::BinaryOp::GreaterThanOrEqual: {
                    interp->return_value =
                        *interp->local_variable_storage.append_object(
                            get_type_descriptor<bool>());
                    *interp->return_value.cast<bool>() =
                        (*first.cast<T>() >= *second.cast<T>());
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
                case MethodTable::UnaryOp::Negate: {
                    interp->return_value =
                        *interp->local_variable_storage.append_object(
                            get_type_descriptor<T>());
                    *interp->return_value.cast<T>() = -*obj.cast<T>();
                    return Fn_OK;
                }
                case MethodTable::UnaryOp::LogicalNot: {
                    interp->return_value =
                        *interp->local_variable_storage.append_object(
                            get_type_descriptor<T>());
                    *interp->return_value.cast<T>() = !*obj.cast<T>();
                    return Fn_OK;
                }
                case MethodTable::UnaryOp::BitComplement: {
                    interp->return_value =
                        *interp->local_variable_storage.append_object(
                            get_type_descriptor<T>());
                    *interp->return_value.cast<T>() = ~*obj.cast<T>();
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

PLY_DEFINE_TYPE_DESCRIPTOR(s8) {
    static TypeDescriptor type_desc{&TypeKey_S8, (s8*) nullptr,
                                    NativeBindings::make<s8>() PLY_METHOD_TABLES_ONLY(
                                        , ArithmeticMethodTable::make<s8>())};
    return &type_desc;
}

PLY_DEFINE_TYPE_DESCRIPTOR(s16) {
    static TypeDescriptor type_desc{&TypeKey_S16, (s16*) nullptr,
                                    NativeBindings::make<s16>() PLY_METHOD_TABLES_ONLY(
                                        , ArithmeticMethodTable::make<s16>())};
    return &type_desc;
}

PLY_DEFINE_TYPE_DESCRIPTOR(s32) {
    static TypeDescriptor type_desc{&TypeKey_S32, (s32*) nullptr,
                                    NativeBindings::make<s32>() PLY_METHOD_TABLES_ONLY(
                                        , ArithmeticMethodTable::make<s32>())};
    return &type_desc;
}

PLY_DEFINE_TYPE_DESCRIPTOR(s64) {
    static TypeDescriptor type_desc{&TypeKey_S64, (s64*) nullptr,
                                    NativeBindings::make<s64>() PLY_METHOD_TABLES_ONLY(
                                        , ArithmeticMethodTable::make<s64>())};
    return &type_desc;
}

PLY_DEFINE_TYPE_DESCRIPTOR(u8) {
    static TypeDescriptor type_desc{&TypeKey_U8, (u8*) nullptr,
                                    NativeBindings::make<u8>() PLY_METHOD_TABLES_ONLY(
                                        , ArithmeticMethodTable::make<u8>())};
    return &type_desc;
}

PLY_DEFINE_TYPE_DESCRIPTOR(u16) {
    static TypeDescriptor type_desc{&TypeKey_U16, (u16*) nullptr,
                                    NativeBindings::make<u16>() PLY_METHOD_TABLES_ONLY(
                                        , ArithmeticMethodTable::make<u16>())};
    return &type_desc;
}

PLY_DEFINE_TYPE_DESCRIPTOR(u32) {
    static TypeDescriptor type_desc{&TypeKey_U32, (u32*) nullptr,
                                    NativeBindings::make<u32>() PLY_METHOD_TABLES_ONLY(
                                        , ArithmeticMethodTable::make<u32>())};
    return &type_desc;
}

PLY_DEFINE_TYPE_DESCRIPTOR(u64) {
    static TypeDescriptor type_desc{&TypeKey_U64, (u64*) nullptr,
                                    NativeBindings::make<u64>() PLY_METHOD_TABLES_ONLY(
                                        , ArithmeticMethodTable::make<u64>())};
    return &type_desc;
}

PLY_DEFINE_TYPE_DESCRIPTOR(float) {
    static TypeDescriptor type_desc{
        &TypeKey_Float, (float*) nullptr, NativeBindings::make<float>(), {}};
    return &type_desc;
}

PLY_DEFINE_TYPE_DESCRIPTOR(double) {
    static TypeDescriptor type_desc{
        &TypeKey_Double, (double*) nullptr, NativeBindings::make<double>(), {}};
    return &type_desc;
}

} // namespace ply

/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
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
    static PLY_INLINE MethodTable make() {
        MethodTable methods;
        methods.binaryOp = [](BaseInterpreter* interp, MethodTable::BinaryOp op,
                              const AnyObject& first, const AnyObject& second) -> FnResult {
            switch (op) {
                case MethodTable::BinaryOp::Multiply: {
                    interp->returnValue =
                        *interp->localVariableStorage.appendObject(getTypeDescriptor<T>());
                    *interp->returnValue.cast<T>() = *first.cast<T>() * *second.cast<T>();
                    return Fn_OK;
                }
                case MethodTable::BinaryOp::Divide: {
                    interp->returnValue =
                        *interp->localVariableStorage.appendObject(getTypeDescriptor<T>());
                    *interp->returnValue.cast<T>() = *first.cast<T>() / *second.cast<T>();
                    return Fn_OK;
                }
                case MethodTable::BinaryOp::Modulo: {
                    interp->returnValue =
                        *interp->localVariableStorage.appendObject(getTypeDescriptor<T>());
                    *interp->returnValue.cast<T>() = *first.cast<T>() % *second.cast<T>();
                    return Fn_OK;
                }
                case MethodTable::BinaryOp::Add: {
                    interp->returnValue =
                        *interp->localVariableStorage.appendObject(getTypeDescriptor<T>());
                    *interp->returnValue.cast<T>() = *first.cast<T>() + *second.cast<T>();
                    return Fn_OK;
                }
                case MethodTable::BinaryOp::Subtract: {
                    interp->returnValue =
                        *interp->localVariableStorage.appendObject(getTypeDescriptor<T>());
                    *interp->returnValue.cast<T>() = *first.cast<T>() - *second.cast<T>();
                    return Fn_OK;
                }
                case MethodTable::BinaryOp::DoubleEqual: {
                    interp->returnValue =
                        *interp->localVariableStorage.appendObject(getTypeDescriptor<bool>());
                    *interp->returnValue.cast<bool>() = (*first.cast<T>() == *second.cast<T>());
                    return Fn_OK;
                }
                case MethodTable::BinaryOp::LessThan: {
                    interp->returnValue =
                        *interp->localVariableStorage.appendObject(getTypeDescriptor<bool>());
                    *interp->returnValue.cast<bool>() = (*first.cast<T>() < *second.cast<T>());
                    return Fn_OK;
                }
                case MethodTable::BinaryOp::LessThanOrEqual: {
                    interp->returnValue =
                        *interp->localVariableStorage.appendObject(getTypeDescriptor<bool>());
                    *interp->returnValue.cast<bool>() = (*first.cast<T>() <= *second.cast<T>());
                    return Fn_OK;
                }
                case MethodTable::BinaryOp::GreaterThan: {
                    interp->returnValue =
                        *interp->localVariableStorage.appendObject(getTypeDescriptor<bool>());
                    *interp->returnValue.cast<bool>() = (*first.cast<T>() > *second.cast<T>());
                    return Fn_OK;
                }
                case MethodTable::BinaryOp::GreaterThanOrEqual: {
                    interp->returnValue =
                        *interp->localVariableStorage.appendObject(getTypeDescriptor<bool>());
                    *interp->returnValue.cast<bool>() = (*first.cast<T>() >= *second.cast<T>());
                    return Fn_OK;
                }
                default: {
                    return MethodTable::unsupportedBinaryOp(interp, op, first, second);
                }
            }
        };
        methods.unaryOp = [](BaseInterpreter* interp, MethodTable::UnaryOp op,
                             const AnyObject& obj) -> FnResult {
            switch (op) {
                case MethodTable::UnaryOp::Negate: {
                    interp->returnValue =
                        *interp->localVariableStorage.appendObject(getTypeDescriptor<T>());
                    *interp->returnValue.cast<T>() = -*obj.cast<T>();
                    return Fn_OK;
                }
                case MethodTable::UnaryOp::LogicalNot: {
                    interp->returnValue =
                        *interp->localVariableStorage.appendObject(getTypeDescriptor<T>());
                    *interp->returnValue.cast<T>() = !*obj.cast<T>();
                    return Fn_OK;
                }
                case MethodTable::UnaryOp::BitComplement: {
                    interp->returnValue =
                        *interp->localVariableStorage.appendObject(getTypeDescriptor<T>());
                    *interp->returnValue.cast<T>() = ~*obj.cast<T>();
                    return Fn_OK;
                }
                default: {
                    return MethodTable::unsupportedUnaryOp(interp, op, obj);
                }
            }
        };
        return methods;
    }
};

#endif // PLY_WITH_METHOD_TABLES

PLY_DEFINE_TYPE_DESCRIPTOR(s8) {
    static TypeDescriptor typeDesc{&TypeKey_S8, (s8*) nullptr,
                                   NativeBindings::make<s8>()
                                       PLY_METHOD_TABLES_ONLY(, ArithmeticMethodTable::make<s8>())};
    return &typeDesc;
}

PLY_DEFINE_TYPE_DESCRIPTOR(s16) {
    static TypeDescriptor typeDesc{
        &TypeKey_S16, (s16*) nullptr,
        NativeBindings::make<s16>() PLY_METHOD_TABLES_ONLY(, ArithmeticMethodTable::make<s16>())};
    return &typeDesc;
}

PLY_DEFINE_TYPE_DESCRIPTOR(s32) {
    static TypeDescriptor typeDesc{
        &TypeKey_S32, (s32*) nullptr,
        NativeBindings::make<s32>() PLY_METHOD_TABLES_ONLY(, ArithmeticMethodTable::make<s32>())};
    return &typeDesc;
}

PLY_DEFINE_TYPE_DESCRIPTOR(s64) {
    static TypeDescriptor typeDesc{
        &TypeKey_S64, (s64*) nullptr,
        NativeBindings::make<s64>() PLY_METHOD_TABLES_ONLY(, ArithmeticMethodTable::make<s64>())};
    return &typeDesc;
}

PLY_DEFINE_TYPE_DESCRIPTOR(u8) {
    static TypeDescriptor typeDesc{&TypeKey_U8, (u8*) nullptr,
                                   NativeBindings::make<u8>()
                                       PLY_METHOD_TABLES_ONLY(, ArithmeticMethodTable::make<u8>())};
    return &typeDesc;
}

PLY_DEFINE_TYPE_DESCRIPTOR(u16) {
    static TypeDescriptor typeDesc{
        &TypeKey_U16, (u16*) nullptr,
        NativeBindings::make<u16>() PLY_METHOD_TABLES_ONLY(, ArithmeticMethodTable::make<u16>())};
    return &typeDesc;
}

PLY_DEFINE_TYPE_DESCRIPTOR(u32) {
    static TypeDescriptor typeDesc{
        &TypeKey_U32, (u32*) nullptr,
        NativeBindings::make<u32>() PLY_METHOD_TABLES_ONLY(, ArithmeticMethodTable::make<u32>())};
    return &typeDesc;
}

PLY_DEFINE_TYPE_DESCRIPTOR(u64) {
    static TypeDescriptor typeDesc{
        &TypeKey_U64, (u64*) nullptr,
        NativeBindings::make<u64>() PLY_METHOD_TABLES_ONLY(, ArithmeticMethodTable::make<u64>())};
    return &typeDesc;
}

PLY_DEFINE_TYPE_DESCRIPTOR(float) {
    static TypeDescriptor typeDesc{
        &TypeKey_Float, (float*) nullptr, NativeBindings::make<float>(), {}};
    return &typeDesc;
}

PLY_DEFINE_TYPE_DESCRIPTOR(double) {
    static TypeDescriptor typeDesc{
        &TypeKey_Double, (double*) nullptr, NativeBindings::make<double>(), {}};
    return &typeDesc;
}

} // namespace ply

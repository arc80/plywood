/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-reflect/Core.h>
#include <ply-reflect/builtin/TypeDescriptor_Arithmetic.h>
#include <ply-reflect/builtin/TypeDescriptor_Bool.h>
#include <ply-reflect/AnyObject.h>

#if PLY_WITH_METHOD_TABLES
#include <ply-reflect/methods/ObjectStack.h>
#endif // PLY_WITH_METHOD_TABLES

namespace ply {

#if PLY_WITH_METHOD_TABLES

struct ArithmeticMethodTable {
    template <typename T>
    static PLY_INLINE MethodTable make() {
        MethodTable methods;
        methods.binaryOp = [](ObjectStack* stack, MethodTable::BinaryOp op, const AnyObject& first,
                              const AnyObject& second) {
            AnyObject* obj;
            switch (op) {
                case MethodTable::BinaryOp::Multiply: {
                    obj = stack->appendObject(getTypeDescriptor<T>());
                    *obj->cast<T>() = *first.cast<T>() * *second.cast<T>();
                    break;
                }
                case MethodTable::BinaryOp::Divide: {
                    obj = stack->appendObject(getTypeDescriptor<T>());
                    *obj->cast<T>() = *first.cast<T>() / *second.cast<T>();
                    break;
                }
                case MethodTable::BinaryOp::Modulo: {
                    obj = stack->appendObject(getTypeDescriptor<T>());
                    *obj->cast<T>() = *first.cast<T>() % *second.cast<T>();
                    break;
                }
                case MethodTable::BinaryOp::Add: {
                    obj = stack->appendObject(getTypeDescriptor<T>());
                    *obj->cast<T>() = *first.cast<T>() + *second.cast<T>();
                    break;
                }
                case MethodTable::BinaryOp::Subtract: {
                    obj = stack->appendObject(getTypeDescriptor<T>());
                    *obj->cast<T>() = *first.cast<T>() - *second.cast<T>();
                    break;
                }
                case MethodTable::BinaryOp::DoubleEqual: {
                    obj = stack->appendObject(getTypeDescriptor<bool>());
                    *obj->cast<bool>() = (*first.cast<T>() == *second.cast<T>());
                    break;
                }
                case MethodTable::BinaryOp::LessThan: {
                    obj = stack->appendObject(getTypeDescriptor<bool>());
                    *obj->cast<bool>() = (*first.cast<T>() < *second.cast<T>());
                    break;
                }
                case MethodTable::BinaryOp::LessThanOrEqual: {
                    obj = stack->appendObject(getTypeDescriptor<bool>());
                    *obj->cast<bool>() = (*first.cast<T>() <= *second.cast<T>());
                    break;
                }
                case MethodTable::BinaryOp::GreaterThan: {
                    obj = stack->appendObject(getTypeDescriptor<bool>());
                    *obj->cast<bool>() = (*first.cast<T>() > *second.cast<T>());
                    break;
                }
                case MethodTable::BinaryOp::GreaterThanOrEqual: {
                    obj = stack->appendObject(getTypeDescriptor<bool>());
                    *obj->cast<bool>() = (*first.cast<T>() >= *second.cast<T>());
                    break;
                }
                default: {
                    PLY_ASSERT(0);
                    break;
                }
            }
            return *obj;
        };
        return methods;
    }
};

#endif // PLY_WITH_METHOD_TABLES

PLY_DEFINE_TYPE_DESCRIPTOR(s8) {
    static TypeDescriptor typeDesc{&TypeKey_S8, sizeof(s8),
                                   NativeBindings::make<s8>()
                                       PLY_METHOD_TABLES_ONLY(, ArithmeticMethodTable::make<s8>())};
    return &typeDesc;
}

PLY_DEFINE_TYPE_DESCRIPTOR(s16) {
    static TypeDescriptor typeDesc{
        &TypeKey_S16, sizeof(s16),
        NativeBindings::make<s16>() PLY_METHOD_TABLES_ONLY(, ArithmeticMethodTable::make<s16>())};
    return &typeDesc;
}

PLY_DEFINE_TYPE_DESCRIPTOR(s32) {
    static TypeDescriptor typeDesc{
        &TypeKey_S32, sizeof(s32),
        NativeBindings::make<s32>() PLY_METHOD_TABLES_ONLY(, ArithmeticMethodTable::make<s32>())};
    return &typeDesc;
}

PLY_DEFINE_TYPE_DESCRIPTOR(s64) {
    static TypeDescriptor typeDesc{
        &TypeKey_S64, sizeof(s64),
        NativeBindings::make<s64>() PLY_METHOD_TABLES_ONLY(, ArithmeticMethodTable::make<s64>())};
    return &typeDesc;
}

PLY_DEFINE_TYPE_DESCRIPTOR(u8) {
    static TypeDescriptor typeDesc{&TypeKey_U8, sizeof(u8),
                                   NativeBindings::make<u8>()
                                       PLY_METHOD_TABLES_ONLY(, ArithmeticMethodTable::make<u8>())};
    return &typeDesc;
}

PLY_DEFINE_TYPE_DESCRIPTOR(u16) {
    static TypeDescriptor typeDesc{
        &TypeKey_U16, sizeof(u16),
        NativeBindings::make<u16>() PLY_METHOD_TABLES_ONLY(, ArithmeticMethodTable::make<u16>())};
    return &typeDesc;
}

PLY_DEFINE_TYPE_DESCRIPTOR(u32) {
    static TypeDescriptor typeDesc{
        &TypeKey_U32, sizeof(u32),
        NativeBindings::make<u32>() PLY_METHOD_TABLES_ONLY(, ArithmeticMethodTable::make<u32>())};
    return &typeDesc;
}

PLY_DEFINE_TYPE_DESCRIPTOR(u64) {
    static TypeDescriptor typeDesc{
        &TypeKey_U64, sizeof(u64),
        NativeBindings::make<u64>() PLY_METHOD_TABLES_ONLY(, ArithmeticMethodTable::make<u64>())};
    return &typeDesc;
}

PLY_DEFINE_TYPE_DESCRIPTOR(float) {
    static TypeDescriptor typeDesc{
        &TypeKey_Float, sizeof(float), NativeBindings::make<float>(), {}};
    return &typeDesc;
}

PLY_DEFINE_TYPE_DESCRIPTOR(double) {
    static TypeDescriptor typeDesc{
        &TypeKey_Double, sizeof(double), NativeBindings::make<double>(), {}};
    return &typeDesc;
}

} // namespace ply

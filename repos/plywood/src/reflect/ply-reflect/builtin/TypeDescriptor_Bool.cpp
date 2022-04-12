/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-reflect/Core.h>
#include <ply-reflect/builtin/TypeDescriptor_Bool.h>
#include <ply-reflect/AnyObject.h>

#if PLY_WITH_METHOD_TABLES
#include <ply-reflect/methods/ObjectStack.h>
#endif // PLY_WITH_METHOD_TABLES

namespace ply {

#if PLY_WITH_METHOD_TABLES

struct BoolMethodTable {
    static PLY_INLINE MethodTable make() {
        MethodTable methods;
        methods.binaryOp = [](ObjectStack* stack, MethodTable::BinaryOp op, const AnyObject& first,
                              const AnyObject& second) {
            AnyObject* obj;
            switch (op) {
                case MethodTable::BinaryOp::DoubleEqual: {
                    obj = stack->appendObject(getTypeDescriptor<bool>());
                    *obj->cast<bool>() = (*first.cast<bool>() == *second.cast<bool>());
                    break;
                }
                case MethodTable::BinaryOp::LogicalAnd: {
                    obj = stack->appendObject(getTypeDescriptor<bool>());
                    *obj->cast<bool>() = (*first.cast<bool>() && *second.cast<bool>());
                    break;
                }
                case MethodTable::BinaryOp::LogicalOr: {
                    obj = stack->appendObject(getTypeDescriptor<bool>());
                    *obj->cast<bool>() = (*first.cast<bool>() || *second.cast<bool>());
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

PLY_DEFINE_TYPE_DESCRIPTOR(bool) {
    static TypeDescriptor typeDesc{&TypeKey_Bool, (bool*) nullptr,
                                   NativeBindings::make<bool>()
                                       PLY_METHOD_TABLES_ONLY(, BoolMethodTable::make())};
    return &typeDesc;
};

} // namespace ply

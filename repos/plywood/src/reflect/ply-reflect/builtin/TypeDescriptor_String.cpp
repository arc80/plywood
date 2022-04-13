/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-reflect/Core.h>
#include <ply-reflect/builtin/TypeDescriptor_String.h>
#include <ply-reflect/builtin/TypeDescriptor_Arithmetic.h>
#include <ply-reflect/AnyObject.h>

#if PLY_WITH_METHOD_TABLES
#include <ply-reflect/methods/ObjectStack.h>
#endif // PLY_WITH_METHOD_TABLES

namespace ply {

#if PLY_WITH_METHOD_TABLES

struct StringMethodTable {
    static PLY_INLINE MethodTable make() {
        MethodTable methods;
        methods.binaryOp = [](ObjectStack* stack, MethodTable::BinaryOp op, const AnyObject& first,
                              const AnyObject& second) {
            AnyObject* obj;
            switch (op) {
                case MethodTable::BinaryOp::Add: {
                    obj = stack->appendObject(getTypeDescriptor<String>());
                    *obj->cast<String>() = *first.cast<String>() + *second.cast<String>();
                    break;
                }
                case MethodTable::BinaryOp::Multiply: {
                    obj = stack->appendObject(getTypeDescriptor<String>());
                    *obj->cast<String>() = *first.cast<String>() * *second.cast<u32>();
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

PLY_DEFINE_TYPE_DESCRIPTOR(String) {
    static TypeDescriptor typeDesc{&TypeKey_String, (String*) nullptr,
                                   NativeBindings::make<String>()
                                       PLY_METHOD_TABLES_ONLY(, StringMethodTable::make())};
    return &typeDesc;
}

} // namespace ply

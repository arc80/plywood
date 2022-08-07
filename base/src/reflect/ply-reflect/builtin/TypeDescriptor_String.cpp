/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-reflect/Core.h>
#include <ply-reflect/builtin/TypeDescriptor_String.h>
#include <ply-reflect/builtin/TypeDescriptor_Arithmetic.h>
#include <ply-reflect/AnyObject.h>

#if PLY_WITH_METHOD_TABLES
#include <ply-reflect/methods/BaseInterpreter.h>
#endif // PLY_WITH_METHOD_TABLES

namespace ply {

#if PLY_WITH_METHOD_TABLES

struct StringMethodTable {
    static PLY_INLINE MethodTable make() {
        MethodTable methods;
        methods.binaryOp = [](BaseInterpreter* interp, MethodTable::BinaryOp op,
                              const AnyObject& first, const AnyObject& second) {
            switch (op) {
                case MethodTable::BinaryOp::Add: {
                    interp->returnValue =
                        *interp->localVariableStorage.appendObject(getTypeDescriptor<String>());
                    *interp->returnValue.cast<String>() =
                        *first.cast<String>() + *second.cast<String>();
                    return MethodResult::OK;
                }
                case MethodTable::BinaryOp::Multiply: {
                    interp->returnValue =
                        *interp->localVariableStorage.appendObject(getTypeDescriptor<String>());
                    *interp->returnValue.cast<String>() =
                        *first.cast<String>() * *second.cast<u32>();
                    return MethodResult::OK;
                }
                default: {
                    return MethodTable::unsupportedBinaryOp(interp, op, first, second);
                }
            }
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

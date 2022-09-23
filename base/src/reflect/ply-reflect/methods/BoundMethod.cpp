/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-reflect/Core.h>
#include <ply-reflect/methods/MethodTable.h>

#if PLY_WITH_METHOD_TABLES

#include <ply-reflect/methods/BoundMethod.h>

namespace ply {

PLY_DEFINE_TYPE_DESCRIPTOR(BoundMethod) {
    static TypeDescriptor_Struct typeDesc{(BoundMethod*) nullptr, "BoundMethod"};
    return &typeDesc;
}

MethodResult BoundMethod_call(BaseInterpreter* interp, const AnyObject& callee,
                              ArrayView<const AnyObject> args) {
    PLY_FORCE_CRASH();
    return MethodResult::OK;
}

Initializer initTypeDescriptorCaller_BoundMethod{[] {
    TypeDescriptor_Struct* structType =
        getTypeDescriptor<BoundMethod>()->cast<TypeDescriptor_Struct>();
    structType->methods.call = BoundMethod_call;
}};

} // namespace ply

#endif // PLY_WITH_METHOD_TABLES

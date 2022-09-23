/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-reflect/Core.h>
#include <ply-reflect/TypeDescriptor_Def.h>

#if PLY_WITH_METHOD_TABLES
#include <ply-reflect/methods/BaseInterpreter.h>

namespace ply {

using Function = MethodResult(BaseInterpreter* base, const AnyObject& self,
                              ArrayView<const AnyObject> args);

PLY_DLL_ENTRY extern TypeKey TypeKey_Function;

PLY_DLL_ENTRY NativeBindings& getNativeBindings_Function();

struct TypeDescriptor_Function : TypeDescriptor {
    PLY_DLL_ENTRY static TypeKey* typeKey;
    Array<TypeDescriptor*> paramTypes;

    PLY_INLINE TypeDescriptor_Function()
        : TypeDescriptor{&TypeKey_Function, (void**) nullptr,
                         getNativeBindings_Function() PLY_METHOD_TABLES_ONLY(, {})} {
    }
};

template <>
struct TypeDescriptorSpecializer<Function> {
    static MethodResult call(BaseInterpreter* interp, const AnyObject& callee,
                             ArrayView<const AnyObject> args);

    static PLY_INLINE TypeDescriptor_Function initType() {
        TypeDescriptor_Function functionType;
        functionType.methods.call = call;
        return functionType;
    }

    static PLY_NO_INLINE TypeDescriptor_Function* get() {
        static TypeDescriptor_Function typeDesc = initType();
        return &typeDesc;
    }
};

} // namespace ply

#endif // PLY_WITH_METHOD_TABLES

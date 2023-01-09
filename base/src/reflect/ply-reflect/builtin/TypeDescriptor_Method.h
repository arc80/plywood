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

struct MethodArgs {
    BaseInterpreter* base;
    AnyObject self;
    ArrayView<const AnyObject> args;
};
using Method = FnResult(const MethodArgs& args);

PLY_DLL_ENTRY extern TypeKey TypeKey_Method;

PLY_DLL_ENTRY NativeBindings& getNativeBindings_Method();

struct TypeDescriptor_Method : TypeDescriptor {
    PLY_DLL_ENTRY static TypeKey* typeKey;

    PLY_INLINE TypeDescriptor_Method()
        : TypeDescriptor{&TypeKey_Method, (void**) nullptr,
                         getNativeBindings_Method() PLY_METHOD_TABLES_ONLY(, {})} {
    }
};

template <>
struct TypeDescriptorSpecializer<Method> {
    static PLY_NO_INLINE TypeDescriptor_Method* get() {
        static TypeDescriptor_Method typeDesc;
        return &typeDesc;
    }
};

} // namespace ply

#endif // PLY_WITH_METHOD_TABLES

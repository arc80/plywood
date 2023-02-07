/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once
#include <ply-reflect/Core.h>
#include <ply-reflect/TypeDescriptor_Def.h>

#if PLY_WITH_METHOD_TABLES
#include <ply-reflect/methods/BaseInterpreter.h>

namespace ply {

struct FnParams {
    BaseInterpreter* base;
    AnyObject self;
    ArrayView<const AnyObject> args;
};

using NativeFunction = FnResult(const FnParams& params);
template <typename T>
using NativeMethod = FnResult(T* self, const FnParams& params);

struct BoundNativeMethod {
    void* self = nullptr;
    NativeMethod<void>* func = nullptr;

    template <typename T>
    BoundNativeMethod(T* obj, NativeMethod<T>* func)
        : self{obj}, func{(NativeMethod<void>*) func} {
    }
};

extern TypeKey TypeKey_NativeFunction;
NativeBindings& get_native_bindings_native_function();
template <>
struct TypeDescriptorSpecializer<NativeFunction> {
    static TypeDescriptor* get() {
        static TypeDescriptor type_desc{&TypeKey_NativeFunction,
                                        (void**) nullptr,
                                        get_native_bindings_native_function(),
                                        {}};
        return &type_desc;
    }
};

extern TypeKey TypeKey_BoundNativeMethod;
NativeBindings& get_native_bindings_bound_native_method();
template <>
struct TypeDescriptorSpecializer<BoundNativeMethod> {
    static TypeDescriptor* get() {
        static TypeDescriptor type_desc{&TypeKey_BoundNativeMethod,
                                        (void**) nullptr,
                                        get_native_bindings_bound_native_method(),
                                        {}};
        return &type_desc;
    }
};

} // namespace ply

#endif // PLY_WITH_METHOD_TABLES

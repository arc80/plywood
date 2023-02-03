/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-reflect/Core.h>
#include <ply-reflect/builtin/TypeDescriptor_Method.h>

#if PLY_WITH_METHOD_TABLES

#include <ply-reflect/AnyObject.h>
#include <ply-reflect/methods/ObjectStack.h>

namespace ply {

NativeBindings& getNativeBindings_NativeFunction() {
    static NativeBindings bindings{
        // create
        [](TypeDescriptor*) -> AnyObject {
            PLY_ASSERT(0); // Not supported
            return {};
        },
        // destroy
        [](AnyObject obj) {},
        // construct
        [](AnyObject obj) { *(void**) obj.data = nullptr; },
        // destruct
        [](AnyObject obj) {},
        // move
        [](AnyObject dst, AnyObject src) {
            PLY_ASSERT(dst.type == src.type);
            *(void**) dst.data = *(void**) src.data;
        },
        // copy
        [](AnyObject dst, const AnyObject src) {
            PLY_ASSERT(dst.type == src.type);
            *(void**) dst.data = *(void**) src.data;
        },
    };
    return bindings;
}

TypeKey TypeKey_NativeFunction{
    // getName
    [](const TypeDescriptor* typeDesc) -> HybridString { //
        return "built-in function";
    },
    // hashDescriptor
    [](Hasher& hasher, const TypeDescriptor* typeDesc) {
        PLY_FORCE_CRASH(); // FIXME
    },
    // equalDescriptors
    [](const TypeDescriptor* type0, const TypeDescriptor* type1) -> bool {
        PLY_FORCE_CRASH(); // FIXME
        return true;
    },
};

NativeBindings& getNativeBindings_BoundNativeMethod() {
    static NativeBindings bindings{
        // create
        [](TypeDescriptor*) -> AnyObject {
            PLY_ASSERT(0); // Not supported
            return {};
        },
        // destroy
        [](AnyObject obj) {},
        // construct
        [](AnyObject obj) { *(void**) obj.data = nullptr; },
        // destruct
        [](AnyObject obj) {},
        // move
        [](AnyObject dst, AnyObject src) {
            PLY_ASSERT(dst.type == src.type);
            *(void**) dst.data = *(void**) src.data;
        },
        // copy
        [](AnyObject dst, const AnyObject src) {
            PLY_ASSERT(dst.type == src.type);
            *(void**) dst.data = *(void**) src.data;
        },
    };
    return bindings;
}

TypeKey TypeKey_BoundNativeMethod{
    // getName
    [](const TypeDescriptor* typeDesc) -> HybridString { //
        return "built-in function";
    },
    // hashDescriptor
    [](Hasher& hasher, const TypeDescriptor* typeDesc) {
        PLY_FORCE_CRASH(); // FIXME
    },
    // equalDescriptors
    [](const TypeDescriptor* type0, const TypeDescriptor* type1) -> bool {
        PLY_FORCE_CRASH(); // FIXME
        return true;
    },
};

} // namespace ply

#endif // PLY_WITH_METHOD_TABLES

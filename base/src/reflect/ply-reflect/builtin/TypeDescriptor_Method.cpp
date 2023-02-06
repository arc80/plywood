/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-reflect/Core.h>
#include <ply-reflect/builtin/TypeDescriptor_Method.h>

#if PLY_WITH_METHOD_TABLES

#include <ply-reflect/AnyObject.h>
#include <ply-reflect/methods/ObjectStack.h>

namespace ply {

NativeBindings& get_native_bindings_native_function() {
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
    // get_name
    [](const TypeDescriptor* type_desc) -> HybridString { //
        return "built-in function";
    },
    // hash_descriptor
    [](Hasher& hasher, const TypeDescriptor* type_desc) {
        PLY_FORCE_CRASH(); // FIXME
    },
    // equal_descriptors
    [](const TypeDescriptor* type0, const TypeDescriptor* type1) -> bool {
        PLY_FORCE_CRASH(); // FIXME
        return true;
    },
};

NativeBindings& get_native_bindings_bound_native_method() {
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
    // get_name
    [](const TypeDescriptor* type_desc) -> HybridString { //
        return "built-in function";
    },
    // hash_descriptor
    [](Hasher& hasher, const TypeDescriptor* type_desc) {
        PLY_FORCE_CRASH(); // FIXME
    },
    // equal_descriptors
    [](const TypeDescriptor* type0, const TypeDescriptor* type1) -> bool {
        PLY_FORCE_CRASH(); // FIXME
        return true;
    },
};

} // namespace ply

#endif // PLY_WITH_METHOD_TABLES

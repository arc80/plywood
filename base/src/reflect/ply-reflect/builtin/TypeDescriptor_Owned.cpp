/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-reflect/Core.h>
#include <ply-reflect/builtin/TypeDescriptor_Owned.h>
#include <ply-reflect/AnyObject.h>

namespace ply {

NativeBindings& get_native_bindings_owned() {
    static NativeBindings bindings{
        // create
        [](TypeDescriptor*) -> AnyObject {
            PLY_ASSERT(0); // Not supported
            return {};
        },
        // destroy
        [](AnyObject obj) {
            PLY_ASSERT(0); // Not supported
        },
        // construct
        [](AnyObject obj) {
            // Note: This is type punning Owned<T> with void*
            *(void**) obj.data = nullptr;
        },
        // destruct
        [](AnyObject obj) {
            TypeDescriptor_Owned* owned_type = obj.type->cast<TypeDescriptor_Owned>();
            // Note: This is type punning Owned<T> with void*
            AnyObject target_obj =
                AnyObject{*(void**) obj.data, owned_type->target_type};
            target_obj.destroy(); // Note: This assumes operator new() uses Heap.alloc
        },
        // move
        [](AnyObject dst, AnyObject src) {
            PLY_ASSERT(dst.type->type_key == &TypeKey_Owned);
            PLY_ASSERT(dst.type == src.type);
            // Note: This is type punning AppOwned<T> with void*
            if (*(void**) dst.data) {
                // Must destroy existing object
                PLY_FORCE_CRASH(); // Unimplemented
            }
            *(void**) dst.data = *(void**) src.data;
            *(void**) src.data = nullptr;
        },
        // copy
        [](AnyObject dst, const AnyObject src) {
            PLY_ASSERT(0); // Not implemented yet
        },
    };
    return bindings;
}

} // namespace ply

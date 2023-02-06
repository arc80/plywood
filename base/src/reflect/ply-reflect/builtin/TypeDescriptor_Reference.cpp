/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-reflect/Core.h>
#include <ply-reflect/builtin/TypeDescriptor_Reference.h>
#include <ply-reflect/AnyObject.h>

namespace ply {

NativeBindings& get_native_bindings_reference() {
    static NativeBindings bindings{
        // create
        [](TypeDescriptor*) -> AnyObject {
            PLY_ASSERT(0); // Not supported
            return {};
        },
        // destroy
        [](AnyObject obj) {
            TypeDescriptor_Reference* reference_type =
                obj.type->cast<TypeDescriptor_Reference>();
            // Note: This is type punning Reference<T> with void*
            void* target = *(void**) obj.data;
            if (target) {
                reference_type->dec_ref(target);
            }
            Heap.free(obj.data);
        },
        // construct
        [](AnyObject obj) {
            // Note: This is type punning Reference<T> with void*
            *(void**) obj.data = nullptr;
        },
        // destruct
        [](AnyObject obj) {
            TypeDescriptor_Reference* reference_type =
                obj.type->cast<TypeDescriptor_Reference>();
            // Note: This is type punning Reference<T> with void*
            void* target = *(void**) obj.data;
            if (target) {
                reference_type->dec_ref(target);
            }
        },
        // move
        [](AnyObject dst, AnyObject src) {
            PLY_ASSERT(dst.type == src.type);
            TypeDescriptor_Reference* reference_type =
                dst.type->cast<TypeDescriptor_Reference>();
            // Note: This is type punning Reference<T> with void*
            void* prev_target = *(void**) dst.data;
            if (prev_target) {
                reference_type->dec_ref(prev_target);
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

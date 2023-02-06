/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-reflect/Core.h>
#include <ply-reflect/builtin/TypeDescriptor_Array.h>
#include <ply-reflect/AnyObject.h>

namespace ply {

NativeBindings& get_native_bindings_array() {
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
        [](AnyObject obj) { new (obj.data) BaseArray; },
        // destruct
        [](AnyObject obj) {
            TypeDescriptor_Array* array_type = obj.type->cast<TypeDescriptor_Array>();
            TypeDescriptor* item_type = array_type->item_type;
            BaseArray* arr = (BaseArray*) obj.data;
            void* item = arr->items;
            u32 item_size = item_type->fixed_size;
            // FIXME: Skip this loop if item_type is trivially
            // destructible (Need a way to determine that)
            for (u32 i = 0; i < arr->num_items; i++) {
                item_type->bindings.destruct({item, item_type});
                item = PLY_PTR_OFFSET(item, item_size);
            }
            arr->~BaseArray();
        },
        // move
        [](AnyObject dst, AnyObject src) {
            PLY_ASSERT(0); // Not implemented yet
        },
        // copy
        [](AnyObject dst, const AnyObject src) {
            PLY_ASSERT(0); // Not implemented yet
        },
    };
    return bindings;
}

} // namespace ply

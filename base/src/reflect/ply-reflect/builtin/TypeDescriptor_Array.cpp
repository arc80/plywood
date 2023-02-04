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

NativeBindings& getNativeBindings_Array() {
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
            TypeDescriptor_Array* arrayType = obj.type->cast<TypeDescriptor_Array>();
            TypeDescriptor* itemType = arrayType->itemType;
            BaseArray* arr = (BaseArray*) obj.data;
            void* item = arr->items;
            u32 itemSize = itemType->fixedSize;
            // FIXME: Skip this loop if itemType is trivially
            // destructible (Need a way to determine that)
            for (u32 i = 0; i < arr->num_items; i++) {
                itemType->bindings.destruct({item, itemType});
                item = PLY_PTR_OFFSET(item, itemSize);
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

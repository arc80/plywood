/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-reflect/Core.h>
#include <ply-reflect/builtin/TypeDescriptor_FixedArray.h>
#include <ply-reflect/builtin/TypeDescriptor_Array.h>
#include <ply-reflect/AnyObject.h>

namespace ply {

NativeBindings& get_native_bindings_fixed_array() {
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
            TypeDescriptor_FixedArray* arr_type =
                obj.type->cast<TypeDescriptor_FixedArray>();
            TypeDescriptor* item_type = arr_type->item_type;
            u32 item_size = item_type->fixed_size;
            void* item = obj.data;
            for (u32 i = 0; i < arr_type->num_items; i++) {
                item_type->bindings.construct({item, item_type});
                item = PLY_PTR_OFFSET(item, item_size);
            }
        },
        // destruct
        [](AnyObject obj) {
            TypeDescriptor_FixedArray* arr_type =
                obj.type->cast<TypeDescriptor_FixedArray>();
            TypeDescriptor* item_type = arr_type->item_type;
            u32 item_size = item_type->fixed_size;
            void* item = obj.data;
            for (u32 i = 0; i < arr_type->num_items; i++) {
                item_type->bindings.destruct({item, item_type});
                item = PLY_PTR_OFFSET(item, item_size);
            }
        },
        // move
        [](AnyObject dst, AnyObject src) {
            PLY_ASSERT(0); // Not implemented yet
        },
        // copy
        [](AnyObject dst, const AnyObject src) {
            TypeDescriptor_FixedArray* dst_arr_type =
                dst.type->cast<TypeDescriptor_FixedArray>();
            TypeDescriptor* dst_item_type = dst_arr_type->item_type;
            if (src.type->type_key == &TypeKey_FixedArray) {
                const TypeDescriptor_FixedArray* src_arr_type =
                    src.type->cast<TypeDescriptor_FixedArray>();
                TypeDescriptor* src_item_type = src_arr_type->item_type;
                // FIXME: Warn about size mismatch
                u32 items_to_copy =
                    min<u32>(dst_arr_type->num_items, src_arr_type->num_items);
                AnyObject dst_item = {dst.data, dst_item_type};
                AnyObject src_item = {src.data, src_item_type};
                while (items_to_copy--) {
                    dst_item.copy(src_item);
                    // FIXME: Support different strides
                    dst_item.data = PLY_PTR_OFFSET(dst_item.data, dst_arr_type->stride);
                    src_item.data = PLY_PTR_OFFSET(src_item.data, src_arr_type->stride);
                }
            } else if (src.type->type_key == &TypeKey_Array) {
                const TypeDescriptor_Array* src_arr_type =
                    src.type->cast<TypeDescriptor_Array>();
                const BaseArray* base_src_array =
                    reinterpret_cast<BaseArray*>(src.data);
                TypeDescriptor* src_item_type = src_arr_type->item_type;
                // FIXME: Warn about size mismatch
                u32 items_to_copy =
                    min<u32>(dst_arr_type->num_items, base_src_array->num_items);
                AnyObject dst_item = {dst.data, dst_item_type};
                AnyObject src_item = {base_src_array->items, src_item_type};
                while (items_to_copy--) {
                    dst_item.copy(src_item);
                    // FIXME: Support different strides
                    dst_item.data = PLY_PTR_OFFSET(dst_item.data, dst_arr_type->stride);
                    src_item.data =
                        PLY_PTR_OFFSET(src_item.data, src_item_type->fixed_size);
                }
            } else {
                PLY_ASSERT(0); // Not implemented yet
            }
        },
    };
    return bindings;
}

} // namespace ply

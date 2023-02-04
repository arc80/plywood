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

NativeBindings& getNativeBindings_FixedArray() {
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
            TypeDescriptor_FixedArray* arrType = obj.type->cast<TypeDescriptor_FixedArray>();
            TypeDescriptor* itemType = arrType->itemType;
            u32 itemSize = itemType->fixedSize;
            void* item = obj.data;
            for (u32 i = 0; i < arrType->numItems; i++) {
                itemType->bindings.construct({item, itemType});
                item = PLY_PTR_OFFSET(item, itemSize);
            }
        },
        // destruct
        [](AnyObject obj) {
            TypeDescriptor_FixedArray* arrType = obj.type->cast<TypeDescriptor_FixedArray>();
            TypeDescriptor* itemType = arrType->itemType;
            u32 itemSize = itemType->fixedSize;
            void* item = obj.data;
            for (u32 i = 0; i < arrType->numItems; i++) {
                itemType->bindings.destruct({item, itemType});
                item = PLY_PTR_OFFSET(item, itemSize);
            }
        },
        // move
        [](AnyObject dst, AnyObject src) {
            PLY_ASSERT(0); // Not implemented yet
        },
        // copy
        [](AnyObject dst, const AnyObject src) {
            TypeDescriptor_FixedArray* dstArrType = dst.type->cast<TypeDescriptor_FixedArray>();
            TypeDescriptor* dstItemType = dstArrType->itemType;
            if (src.type->typeKey == &TypeKey_FixedArray) {
                const TypeDescriptor_FixedArray* srcArrType =
                    src.type->cast<TypeDescriptor_FixedArray>();
                TypeDescriptor* srcItemType = srcArrType->itemType;
                // FIXME: Warn about size mismatch
                u32 itemsToCopy = min<u32>(dstArrType->numItems, srcArrType->numItems);
                AnyObject dstItem = {dst.data, dstItemType};
                AnyObject srcItem = {src.data, srcItemType};
                while (itemsToCopy--) {
                    dstItem.copy(srcItem);
                    // FIXME: Support different strides
                    dstItem.data = PLY_PTR_OFFSET(dstItem.data, dstArrType->stride);
                    srcItem.data = PLY_PTR_OFFSET(srcItem.data, srcArrType->stride);
                }
            } else if (src.type->typeKey == &TypeKey_Array) {
                const TypeDescriptor_Array* srcArrType = src.type->cast<TypeDescriptor_Array>();
                const BaseArray* baseSrcArray = reinterpret_cast<BaseArray*>(src.data);
                TypeDescriptor* srcItemType = srcArrType->itemType;
                // FIXME: Warn about size mismatch
                u32 itemsToCopy = min<u32>(dstArrType->numItems, baseSrcArray->num_items);
                AnyObject dstItem = {dst.data, dstItemType};
                AnyObject srcItem = {baseSrcArray->items, srcItemType};
                while (itemsToCopy--) {
                    dstItem.copy(srcItem);
                    // FIXME: Support different strides
                    dstItem.data = PLY_PTR_OFFSET(dstItem.data, dstArrType->stride);
                    srcItem.data = PLY_PTR_OFFSET(srcItem.data, srcItemType->fixedSize);
                }
            } else {
                PLY_ASSERT(0); // Not implemented yet
            }
        },
    };
    return bindings;
}

} // namespace ply

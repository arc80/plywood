/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-reflect/Core.h>
#include <ply-reflect/builtin/TypeDescriptor_EnumIndexedArray.h>
#include <ply-reflect/AnyObject.h>

namespace ply {

NativeBindings& getNativeBindings_EnumIndexedArray() {
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
            TypeDescriptor_EnumIndexedArray* arrType =
                obj.type->cast<TypeDescriptor_EnumIndexedArray>();
            TypeDescriptor* itemType = arrType->itemType;
            u32 itemSize = itemType->fixedSize;
            u32 count = arrType->enumType->identifiers.numItems();
            PLY_ASSERT(itemSize * count == arrType->fixedSize);
            void* item = obj.data;
            for (u32 i = 0; i < count; i++) {
                PLY_UNUSED(i);
                itemType->bindings.construct({item, itemType});
                item = PLY_PTR_OFFSET(item, itemSize);
            }
        },
        // destruct
        [](AnyObject obj) {
            TypeDescriptor_EnumIndexedArray* arrType =
                obj.type->cast<TypeDescriptor_EnumIndexedArray>();
            TypeDescriptor* itemType = arrType->itemType;
            u32 itemSize = itemType->fixedSize;
            u32 count = arrType->enumType->identifiers.numItems();
            PLY_ASSERT(itemSize * count == arrType->fixedSize);
            void* item = obj.data;
            for (u32 i = 0; i < count; i++) {
                PLY_UNUSED(i);
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
            PLY_ASSERT(0); // Not implemented yet
        },
    };
    return bindings;
}

} // namespace ply

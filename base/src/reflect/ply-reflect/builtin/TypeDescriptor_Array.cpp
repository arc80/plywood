/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
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
        [](AnyObject obj) { new (obj.data) impl::BaseArray; },
        // destruct
        [](AnyObject obj) {
            TypeDescriptor_Array* arrayType = obj.type->cast<TypeDescriptor_Array>();
            TypeDescriptor* itemType = arrayType->itemType;
            impl::BaseArray* arr = (impl::BaseArray*) obj.data;
            void* item = arr->m_items;
            u32 itemSize = itemType->fixedSize;
            // FIXME: Skip this loop if itemType is trivially
            // destructible (Need a way to determine that)
            for (u32 i : range(arr->m_numItems)) {
                PLY_UNUSED(i);
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

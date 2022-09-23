/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-reflect/Core.h>
#include <ply-reflect/builtin/TypeDescriptor_Owned.h>
#include <ply-reflect/AnyObject.h>

namespace ply {

NativeBindings& getNativeBindings_Owned() {
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
            TypeDescriptor_Owned* ownedType = obj.type->cast<TypeDescriptor_Owned>();
            // Note: This is type punning Owned<T> with void*
            AnyObject targetObj = AnyObject{*(void**) obj.data, ownedType->targetType};
            targetObj.destroy(); // Note: This assumes operator new() uses PLY_HEAP.alloc
        },
        // move
        [](AnyObject dst, AnyObject src) {
            PLY_ASSERT(dst.type->typeKey == &TypeKey_Owned);
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

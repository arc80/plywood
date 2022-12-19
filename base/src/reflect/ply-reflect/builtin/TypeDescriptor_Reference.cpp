/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-reflect/Core.h>
#include <ply-reflect/builtin/TypeDescriptor_Reference.h>
#include <ply-reflect/AnyObject.h>

namespace ply {

NativeBindings& getNativeBindings_Reference() {
    static NativeBindings bindings{
        // create
        [](TypeDescriptor*) -> AnyObject {
            PLY_ASSERT(0); // Not supported
            return {};
        },
        // destroy
        [](AnyObject obj) {
            TypeDescriptor_Reference* referenceType = obj.type->cast<TypeDescriptor_Reference>();
            // Note: This is type punning Reference<T> with void*
            void* target = *(void**) obj.data;
            if (target) {
                referenceType->decRef(target);
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
            TypeDescriptor_Reference* referenceType = obj.type->cast<TypeDescriptor_Reference>();
            // Note: This is type punning Reference<T> with void*
            void* target = *(void**) obj.data;
            if (target) {
                referenceType->decRef(target);
            }
        },
        // move
        [](AnyObject dst, AnyObject src) {
            PLY_ASSERT(dst.type == src.type);
            TypeDescriptor_Reference* referenceType = dst.type->cast<TypeDescriptor_Reference>();
            // Note: This is type punning Reference<T> with void*
            void* prevTarget = *(void**) dst.data;
            if (prevTarget) {
                referenceType->decRef(prevTarget);
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

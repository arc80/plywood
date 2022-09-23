/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-reflect/Core.h>
#include <ply-reflect/builtin/TypeDescriptor_Enum.h>
#include <ply-reflect/AnyObject.h>

namespace ply {

NativeBindings& getNativeBindings_Enum() {
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
        [](AnyObject obj) {},
        // destruct
        [](AnyObject obj) {},
        // move
        [](AnyObject dst, AnyObject src) {
            PLY_ASSERT(dst.type->typeKey == &TypeKey_Enum);
            PLY_ASSERT(dst.type == src.type);
            TypeDescriptor_Enum* enumType = dst.type->cast<TypeDescriptor_Enum>();
            if (enumType->fixedSize == 1) {
                *(u8*) dst.data = *(u8*) src.data;
            } else if (enumType->fixedSize == 2) {
                *(u16*) dst.data = *(u16*) src.data;
            } else if (enumType->fixedSize == 4) {
                *(u32*) dst.data = *(u32*) src.data;
            } else {
                PLY_ASSERT(0);
            }
        },
        // copy
        [](AnyObject dst, const AnyObject src) {
            PLY_ASSERT(0); // Not implemented yet
        },
    };
    return bindings;
}

} // namespace ply

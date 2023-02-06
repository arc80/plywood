/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-reflect/Core.h>
#include <ply-reflect/builtin/TypeDescriptor_Enum.h>
#include <ply-reflect/AnyObject.h>

namespace ply {

NativeBindings& get_native_bindings_enum() {
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
            PLY_ASSERT(dst.type->type_key == &TypeKey_Enum);
            PLY_ASSERT(dst.type == src.type);
            TypeDescriptor_Enum* enum_type = dst.type->cast<TypeDescriptor_Enum>();
            if (enum_type->fixed_size == 1) {
                *(u8*) dst.data = *(u8*) src.data;
            } else if (enum_type->fixed_size == 2) {
                *(u16*) dst.data = *(u16*) src.data;
            } else if (enum_type->fixed_size == 4) {
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

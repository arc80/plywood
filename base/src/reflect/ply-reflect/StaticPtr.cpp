/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-reflect/Core.h>
#include <ply-reflect/StaticPtr.h>
#include <ply-reflect/TypeKey.h>

namespace ply {

SLOG_DECLARE_CHANNEL(Load)

NativeBindings& get_native_bindings_static_ptr() {
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
        [](AnyObject obj) { //
            new (obj.data) StaticPtr<void>;
        },
        // destruct
        [](AnyObject obj) {},
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

TypeKey TypeKey_StaticPtr = {
    // get_name
    [](const TypeDescriptor* type_desc) -> HybridString { //
        // FIXME: Include template argument
        return "StaticPtr";
    },

    // hash_descriptor
    TypeKey::hash_empty_descriptor,

    // equal_descriptors
    TypeKey::always_equal_descriptors,
};

} // namespace ply

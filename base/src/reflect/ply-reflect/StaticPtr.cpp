/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-reflect/Core.h>
#include <ply-reflect/StaticPtr.h>
#include <ply-reflect/TypeKey.h>

namespace ply {

SLOG_DECLARE_CHANNEL(Load)

NativeBindings& getNativeBindings_StaticPtr() {
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
    // getName
    [](const TypeDescriptor* typeDesc) -> HybridString { //
        // FIXME: Include template argument
        return "StaticPtr";
    },

    // hashDescriptor
    TypeKey::hashEmptyDescriptor,

    // equalDescriptors
    TypeKey::alwaysEqualDescriptors,
};

} // namespace ply

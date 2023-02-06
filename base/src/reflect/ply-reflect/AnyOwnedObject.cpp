/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-reflect/Core.h>
#include <ply-reflect/TypeDescriptor_Def.h>
#include <ply-reflect/AnyOwnedObject.h>

namespace ply {

SLOG_DECLARE_CHANNEL(Load)

TypeKey TypeKey_AnyOwnedObject{
    // get_name
    [](const TypeDescriptor* type_desc) -> HybridString { //
        return "AnyOwnedObject";
    },

    // hash_descriptor
    TypeKey::hash_empty_descriptor,

    // equal_descriptors
    TypeKey::always_equal_descriptors,
};

PLY_DEFINE_TYPE_DESCRIPTOR(AnyOwnedObject) {
    static TypeDescriptor type_desc{&TypeKey_AnyOwnedObject, (AnyOwnedObject*) nullptr,
                                    NativeBindings::make<AnyOwnedObject>()
                                        PLY_METHOD_TABLES_ONLY(, {})};
    return &type_desc;
}

} // namespace ply

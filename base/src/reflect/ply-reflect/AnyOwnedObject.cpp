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
    // getName
    [](const TypeDescriptor* typeDesc) -> HybridString { //
        return "AnyOwnedObject";
    },

    // hashDescriptor
    TypeKey::hashEmptyDescriptor,

    // equalDescriptors
    TypeKey::alwaysEqualDescriptors,
};

PLY_DEFINE_TYPE_DESCRIPTOR(AnyOwnedObject) {
    static TypeDescriptor typeDesc{&TypeKey_AnyOwnedObject, (AnyOwnedObject*) nullptr,
                                   NativeBindings::make<AnyOwnedObject>()
                                       PLY_METHOD_TABLES_ONLY(, {})};
    return &typeDesc;
}

} // namespace ply

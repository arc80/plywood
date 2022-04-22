/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-reflect/Core.h>
#include <ply-reflect/TypeDescriptor_Def.h>
#include <ply-reflect/PersistRead.h>
#include <ply-reflect/PersistWrite.h>
#include <ply-reflect/AnyOwnedObject.h>

namespace ply {

SLOG_DECLARE_CHANNEL(Load)

TypeKey TypeKey_AnyOwnedObject{
    // getName
    [](const TypeDescriptor* typeDesc) -> HybridString { //
        return "AnyOwnedObject";
    },

    // write
    [](AnyObject obj, WriteObjectContext* context) {
        AnyOwnedObject* ownTypedPtr = (AnyOwnedObject*) obj.data;
        TypeDescriptor* targetType = ownTypedPtr->type;

        // Make sure the target's TypeDescriptor is written as part of the schema.
        // Assign and write its formatID.
        u32 targetFormatID = (u32) FormatKey::None;
        if (targetType) {
            targetFormatID = context->writeFormatContext->addOrGetFormatID(targetType);
        }
        context->out.write<u32>(targetFormatID);

        // Write the target
        if (targetType) {
            targetType->typeKey->write(*ownTypedPtr, context);
        }
    },

    // writeFormat
    [](TypeDescriptor*, WriteFormatContext* context) { //
        context->writePrimitive(FormatKey::Typed);
    },

    // read
    [](AnyObject obj, ReadObjectContext* context, FormatDescriptor* formatDesc) {
        if ((FormatKey) formatDesc->formatKey != FormatKey::Typed) {
            SLOG(Load, "Can't load AnyOwnedObject");
            skip(context, formatDesc);
            return;
        }

        AnyOwnedObject* ownTypedPtr = (AnyOwnedObject*) obj.data;

        // Read formatID and look up the corresponding FormatDescriptor of the target.
        u32 targetFormatID = context->in.read<u32>();
        if (targetFormatID == (u32) FormatKey::None) {
            *ownTypedPtr = AnyOwnedObject{};
        } else {
            FormatDescriptor* targetFormat = context->schema->getFormatDesc(targetFormatID);

            // Find built-in type.
            TypeDescriptor* targetType = context->typeResolver->getType(targetFormat);

            // Read the target
            *ownTypedPtr = AnyObject::create(targetType);
            targetType->typeKey->read(*ownTypedPtr, context, targetFormat);
        }
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

/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-reflect/Core.h>
#include <ply-reflect/TypeDescriptor.h>
#include <ply-reflect/PersistRead.h>
#include <ply-reflect/PersistWrite.h>

namespace ply {

SLOG_DECLARE_CHANNEL(Load)

TypeKey TypeKey_OwnTypedPtr{
    // write
    [](TypedPtr obj, WriteObjectContext* context) {
        OwnTypedPtr* ownTypedPtr = (OwnTypedPtr*) obj.ptr;
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
    [](TypedPtr obj, ReadObjectContext* context, FormatDescriptor* formatDesc) {
        if ((FormatKey) formatDesc->formatKey != FormatKey::Typed) {
            SLOG(Load, "Can't load OwnTypedPtr");
            skip(context, formatDesc);
            return;
        }

        OwnTypedPtr* ownTypedPtr = (OwnTypedPtr*) obj.ptr;

        // Read formatID and look up the corresponding FormatDescriptor of the target.
        u32 targetFormatID = context->in.read<u32>();
        if (targetFormatID == (u32) FormatKey::None) {
            *ownTypedPtr = OwnTypedPtr{};
        } else {
            FormatDescriptor* targetFormat = context->schema->getFormatDesc(targetFormatID);

            // Find built-in type.
            TypeDescriptor* targetType = context->typeResolver->getType(targetFormat);

            // Read the target
            *ownTypedPtr = TypedPtr::create(targetType);
            targetType->typeKey->read(*ownTypedPtr, context, targetFormat);
        }
    },

    // hashDescriptor
    TypeKey::hashEmptyDescriptor,

    // equalDescriptors
    TypeKey::alwaysEqualDescriptors,
};

PLY_NO_INLINE TypeDescriptor* TypeResolver<OwnTypedPtr>::get() {
    static TypeDescriptor typeDesc{&TypeKey_OwnTypedPtr, sizeof(OwnTypedPtr),
                                   NativeBindings::make<OwnTypedPtr>()};
    return &typeDesc;
}

} // namespace ply

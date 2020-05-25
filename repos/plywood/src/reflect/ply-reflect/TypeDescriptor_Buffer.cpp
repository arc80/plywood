/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-reflect/Core.h>
#include <ply-reflect/TypeKey.h>
#include <ply-runtime/container/Boxed.h>
#include <ply-reflect/PersistRead.h>
#include <ply-reflect/PersistWrite.h>

namespace ply {

SLOG_DECLARE_CHANNEL(Load)

TypeKey TypeKey_Buffer{
    // write
    [](TypedPtr obj, WriteObjectContext* context) {
        Boxed<Buffer>::write(context->out, *(Buffer*) obj.ptr);
    },
    // writeFormat
    [](TypeDescriptor*, WriteFormatContext* context) {
        context->writeArray(TypeResolver<u8>::get());
    },
    // read
    [](TypedPtr obj, ReadObjectContext* context, FormatDescriptor* formatDesc) {
        PLY_ASSERT(obj.type == TypeResolver<Buffer>::get());
        if ((FormatKey) formatDesc->formatKey == FormatKey::Array) {
            FormatDescriptor_Array* arrayFormat = (FormatDescriptor_Array*) formatDesc;
            if (arrayFormat->itemFormat == &FormatDescriptor_U8) {
                *(Buffer*) obj.ptr = Boxed<Buffer>::read(context->in);
                return;
            }
        }
        SLOG(Load, "Can't load Buffer");
        skip(context, formatDesc);
    },
    // hashDescriptor
    TypeKey::hashEmptyDescriptor,
    // equalDescriptors
    TypeKey::alwaysEqualDescriptors,
};

PLY_NO_INLINE TypeDescriptor* TypeResolver<Buffer>::get() {
    static TypeDescriptor typeDesc{&TypeKey_Buffer, sizeof(Buffer), NativeBindings::make<Buffer>()};
    return &typeDesc;
}

} // namespace ply

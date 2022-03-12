/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-reflect/Core.h>
#include <ply-reflect/StaticPtr.h>
#include <ply-reflect/TypeKey.h>
#include <ply-reflect/PersistRead.h>
#include <ply-reflect/PersistWrite.h>

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
    // write
    [](AnyObject obj, WriteObjectContext* context) {
        TypeDescriptor_StaticPtr* staticPtrType = obj.type->cast<TypeDescriptor_StaticPtr>();
        const BaseStaticPtr::PossibleValues* pvals = staticPtrType->possibleValues;
        void* staticPtrValue = *(void**) obj.data;
        // Note: This could be optimized using a hash map. Not sure whether to add hashmap to
        // TypeDescriptor_StaticPtr, or to generate one at serialization time.
        u32 serializedValue = 0;
        for (u32 i = 0; i < pvals->ptrValues.numItems(); i++) {
            if (pvals->ptrValues[i] == staticPtrValue) {
                serializedValue = i;
                context->out.write<u32>(serializedValue);
                return; // Success
            }
        }
        PLY_ASSERT(0); // Value not found; unable to serialize
    },

    // writeFormat
    [](TypeDescriptor* typeDesc, WriteFormatContext* context) {
        TypeDescriptor_StaticPtr* staticPtrType = typeDesc->cast<TypeDescriptor_StaticPtr>();
        const BaseStaticPtr::PossibleValues* pvals = staticPtrType->possibleValues;
        PLY_ASSERT(pvals->enumeratorNames.numItems() == pvals->ptrValues.numItems());
        context->beginEnum({}, pvals->enumeratorNames.numItems(), 4);
        for (StringView enumeratorName : pvals->enumeratorNames) {
            context->writeEnumEntry(enumeratorName);
        }
        context->endEnum();
    },

    // read
    [](AnyObject obj, ReadObjectContext* context, FormatDescriptor* formatDesc) {
        NativeEndianReader& in = context->in;
        FormatDescriptor_Enum* enumFormat = (FormatDescriptor_Enum*) formatDesc;
        TypeDescriptor_StaticPtr* staticPtrType = obj.type->cast<TypeDescriptor_StaticPtr>();
        const BaseStaticPtr::PossibleValues* pvals = staticPtrType->possibleValues;
        PLY_ASSERT(pvals->enumeratorNames.numItems() == pvals->ptrValues.numItems());
        // Note: This could (and should) be optimized, like many other readers, by adding
        // another loading pass after the formatdescriptors are read but before the data is read,
        // and building translation tables at that time.
        u32 serializedValue = in.read<u32>();
        if (serializedValue >= enumFormat->identifiers.numItems()) {
            SLOG(Load, "Serialized enum value out of range reading StaticPtr");
            return;
        }
        StringView serializedName = enumFormat->identifiers[serializedValue];
        for (u32 i = 0; i < pvals->enumeratorNames.numItems(); i++) {
            if (pvals->enumeratorNames[i] == serializedName) {
                *(void**) obj.data = pvals->ptrValues[i];
                return; // Success
            }
        }
        SLOG(Load, "Can't convert enum value \"{}\" while loading StaticPtr", serializedName);
    },

    // hashDescriptor
    TypeKey::hashEmptyDescriptor,

    // equalDescriptors
    TypeKey::alwaysEqualDescriptors,
};

} // namespace ply

/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-reflect/Core.h>
#include <ply-reflect/PersistRead.h>
#include <ply-reflect/TypeKey.h>
#include <ply-runtime/container/Boxed.h>
#include <map>

namespace ply {

// Note: Many FormatDescriptor* in this file (and others) could use a "const" qualifier. Worth
// fixing?

void skip(ReadObjectContext* context, FormatDescriptor* formatDesc) {
    switch ((FormatKey) formatDesc->formatKey) {
        case FormatKey::Bool:
            context->in.read<bool>();
            break;
        case FormatKey::S8:
            context->in.read<s8>();
            break;
        case FormatKey::S16:
            context->in.read<s16>();
            break;
        case FormatKey::S32:
            context->in.read<s32>();
            break;
        case FormatKey::S64:
            context->in.read<s64>();
            break;
        case FormatKey::U8:
            context->in.read<u8>();
            break;
        case FormatKey::U16:
            context->in.read<u16>();
            break;
        case FormatKey::U32:
            context->in.read<u32>();
            break;
        case FormatKey::U64: {
            context->in.read<u64>();
            break;
        }
        case FormatKey::Float: {
            context->in.read<float>();
            break;
        }
        case FormatKey::Double: {
            context->in.read<double>();
            break;
        }
        case FormatKey::String: {
            Boxed<String>::read(context->in);
            break;
        }
        case FormatKey::TypedArray: {
            u32 formatID = context->in.read<u32>();
            FormatDescriptor* formatDesc = context->schema->getFormatDesc(formatID);
            u32 numItems = context->in.read<u32>();
            for (u32 i : range(numItems)) {
                PLY_UNUSED(i);
                skip(context, formatDesc);
            }
            break;
        }
        case FormatKey::Typed: {
            u32 formatID = context->in.read<u32>();
            FormatDescriptor* formatDesc = context->schema->getFormatDesc(formatID);
            skip(context, formatDesc);
            break;
        }
        case FormatKey::FixedArray: {
            FormatDescriptor_FixedArray* fixedFormat = (FormatDescriptor_FixedArray*) formatDesc;
            for (u32 i : range(fixedFormat->numItems)) {
                PLY_UNUSED(i);
                skip(context, fixedFormat->itemFormat);
            }
            break;
        }
        case FormatKey::Array: {
            FormatDescriptor_Array* arrayFormat = (FormatDescriptor_Array*) formatDesc;
            u32 numItems = context->in.read<u32>();
            for (u32 i : range(numItems)) {
                PLY_UNUSED(i);
                skip(context, arrayFormat->itemFormat);
            }
            break;
        }
        case FormatKey::Owned: {
            FormatDescriptor_Owned* ownedFormat = (FormatDescriptor_Owned*) formatDesc;
            skip(context, ownedFormat->childFormat);
            break;
        }
        case FormatKey::Struct: {
            FormatDescriptor_Struct* structFormat = (FormatDescriptor_Struct*) formatDesc;
            for (FormatDescriptor_Struct::Member& member : structFormat->members) {
                skip(context, member.formatDesc);
            }
            break;
        }
        case FormatKey::Enum: {
            FormatDescriptor_Enum* enumFormat = (FormatDescriptor_Enum*) formatDesc;
            PLY_ASSERT(enumFormat->fixedSize <= 4);
            // FIXME: Could implement an InStream::skip function instead of this unsafe code:
            u32 value;
            context->in.ins->read({(char*) &value, enumFormat->fixedSize});
            break;
        }
        case FormatKey::EnumIndexedArray: {
            FormatDescriptor_EnumIndexedArray* enumIndexedFormat =
                (FormatDescriptor_EnumIndexedArray*) formatDesc;
            for (u32 i : range(enumIndexedFormat->enumFormat->identifiers.numItems())) {
                PLY_UNUSED(i);
                skip(context, enumIndexedFormat->itemFormat);
            }
            break;
        }
        case FormatKey::Switch: {
            FormatDescriptor_Switch* switchFormat = (FormatDescriptor_Switch*) formatDesc;
            u16 id = context->in.read<u16>();
            skip(context, switchFormat->states[id].structFormat);
            break;
        }
        default: {
            PLY_ASSERT(0);
        }
    }
}

void readLinkTable(NativeEndianReader* in, LoadPtrResolver* ptrResolver) {
    u32 numLinkItems = in->read<u32>();
    ptrResolver->linkTable.resize(numLinkItems);
    for (u32 i = 0; i < numLinkItems; i++) {
        ptrResolver->linkTable[i].fileOffset = in->read<u32>();
    }
}

TypedPtr readObject(ReadObjectContext* context) {
    TypedPtr obj; // Zero-initialized

    // Read formatID
    u32 formatID = context->in.read<u32>();
    if (!context->in.ins->atEOF()) { // EOF test; ensures formatID is valid
        FormatDescriptor* formatDesc = context->schema->getFormatDesc(formatID);

        // Get TypeDescriptor
        TypeDescriptor* typeDesc = context->typeResolver->getType(formatDesc);

        // Instantiate object
        obj = TypedPtr::create(typeDesc);

        // Load into the object
        obj.type->typeKey->read(obj, context, formatDesc);
    }

    return obj;
}

void resolveLinks(LoadPtrResolver* ptrResolver) {
    PLY_ASSERT(ptrResolver->linkTableIndex <= ptrResolver->linkTable.numItems());
    while (ptrResolver->linkTableIndex < ptrResolver->linkTable.numItems()) {
        // Object was likely skipped
        ptrResolver->linkTable[ptrResolver->linkTableIndex].ptr = nullptr;
        ptrResolver->linkTableIndex++;
    }

    for (const LoadPtrResolver::WeakPtrToResolve& weakInfo : ptrResolver->weakPtrsToResolve) {
        u32 linkIndex = weakInfo.weakPtr->linkIndex;
        // FIXME: handle bad data gracefully
        PLY_ASSERT(linkIndex < ptrResolver->linkTable.numItems());
        const LoadPtrResolver::LinkTableEntry& linkEntry = ptrResolver->linkTable[linkIndex];
#if PLY_VALIDATE_RESOLVED_PTR_TYPES
        // FIXME: handle bad data gracefully
        PLY_ASSERT(linkEntry.typeDesc == weakInfo.typeDesc);
#endif
        PLY_ASSERT(linkEntry.ptr); // We don't support null ptrs at save time yet
        weakInfo.weakPtr->ptr = linkEntry.ptr;
    }
}

} // namespace ply

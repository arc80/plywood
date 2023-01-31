/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-reflect/Core.h>
#include <ply-reflect/TypeKey.h>
#include <ply-reflect/FormatDescriptor.h>
#include <ply-reflect/PersistRead.h>
#include <ply-reflect/PersistWrite.h>
#include <ply-runtime/container/Numeric.h>
#include <ply-runtime/container/Boxed.h>
#include <ply-runtime/Algorithm.h>

namespace ply {

SLOG_CHANNEL(Load, "Load")

void readNumeric(AnyObject obj, ReadObjectContext* context, FormatDescriptor* formatDesc) {
    NativeEndianReader& in = context->in;
    Numeric value;
    switch ((FormatKey) formatDesc->formatKey) {
        case FormatKey::Bool:
            value = in.read<bool>();
            break;
        case FormatKey::S8:
            value = in.read<s8>();
            break;
        case FormatKey::S16:
            value = in.read<s16>();
            break;
        case FormatKey::S32:
            value = in.read<s32>();
            break;
        case FormatKey::S64:
            value = in.read<s64>();
            break;
        case FormatKey::U8:
            value = in.read<u8>();
            break;
        case FormatKey::U16:
            value = in.read<u16>();
            break;
        case FormatKey::U32:
            value = in.read<u32>();
            break;
        case FormatKey::U64:
            value = in.read<u64>();
            break;
        case FormatKey::Float:
            value = in.read<float>();
            break;
        case FormatKey::Double:
            value = in.read<double>();
            break;
        default: {
            SLOG(Load, "Can't convert to numeric");
            skip(context, formatDesc);
            return;
        }
    }
    bool precise = true;
    TypeKey* typeKey = obj.type->typeKey;
    // FIXME: Find a way to convert to a switch
    if (typeKey == &TypeKey_Bool) {
        u32 temp = value.cast<u32>(precise);
        bool value = (temp != 0);
        precise = precise && ((u32) value == temp);
        *((bool*) obj.data) = value;
    } else if (typeKey == &TypeKey_S8) {
        *((s8*) obj.data) = value.cast<s8>(precise);
    } else if (typeKey == &TypeKey_S16) {
        *((s16*) obj.data) = value.cast<s16>(precise);
    } else if (typeKey == &TypeKey_S32) {
        *((s32*) obj.data) = value.cast<s32>(precise);
    } else if (typeKey == &TypeKey_S64) {
        *((s64*) obj.data) = value.cast<s64>(precise);
    } else if (typeKey == &TypeKey_U8) {
        *((u8*) obj.data) = value.cast<u8>(precise);
    } else if (typeKey == &TypeKey_U16) {
        *((u16*) obj.data) = value.cast<u16>(precise);
    } else if (typeKey == &TypeKey_U32) {
        *((u32*) obj.data) = value.cast<u32>(precise);
    } else if (typeKey == &TypeKey_U64) {
        *((u64*) obj.data) = value.cast<u64>(precise);
    } else if (typeKey == &TypeKey_Float) {
        *((float*) obj.data) = value.cast<float>(precise);
    } else if (typeKey == &TypeKey_Double) {
        *((double*) obj.data) = value.cast<double>(precise);
    } else {
        PLY_ASSERT(0);
    }
    if (!precise) {
        // FIXME: Support Numeric in String::Format
        // FIXME: Write names of types, context in Type/FormatDescriptors
        SLOG(Load, "Loss of precision loading {}", value.cast<double>());
    }
}

//-----------------------------------------------------------------
// Primitive TypeKeys
//
TypeKey TypeKey_Bool{
    [](const TypeDescriptor* typeDesc) -> HybridString { return "bool"; },
    [](AnyObject obj, WriteObjectContext* context) { context->out.write<bool>(*(bool*) obj.data); },
    [](TypeDescriptor*, WriteFormatContext* context) { context->writePrimitive(FormatKey::Bool); },
    readNumeric, // FIXME
    TypeKey::hashEmptyDescriptor,
    TypeKey::alwaysEqualDescriptors,
};

TypeKey TypeKey_S8{
    [](const TypeDescriptor* typeDesc) -> HybridString { return "s8"; },
    [](AnyObject obj, WriteObjectContext* context) { context->out.write<s8>(*(s8*) obj.data); },
    [](TypeDescriptor*, WriteFormatContext* context) { context->writePrimitive(FormatKey::S8); },
    readNumeric,
    TypeKey::hashEmptyDescriptor,
    TypeKey::alwaysEqualDescriptors,
};

TypeKey TypeKey_S16{
    [](const TypeDescriptor* typeDesc) -> HybridString { return "s16"; },
    [](AnyObject obj, WriteObjectContext* context) { context->out.write<s16>(*(s16*) obj.data); },
    [](TypeDescriptor*, WriteFormatContext* context) { context->writePrimitive(FormatKey::S16); },
    readNumeric,
    TypeKey::hashEmptyDescriptor,
    TypeKey::alwaysEqualDescriptors,
};

TypeKey TypeKey_S32{
    [](const TypeDescriptor* typeDesc) -> HybridString { return "s32"; },
    [](AnyObject obj, WriteObjectContext* context) { context->out.write<s32>(*(s32*) obj.data); },
    [](TypeDescriptor*, WriteFormatContext* context) { context->writePrimitive(FormatKey::S32); },
    readNumeric,
    TypeKey::hashEmptyDescriptor,
    TypeKey::alwaysEqualDescriptors,
};

TypeKey TypeKey_S64{
    [](const TypeDescriptor* typeDesc) -> HybridString { return "s64"; },
    [](AnyObject obj, WriteObjectContext* context) { context->out.write<s64>(*(s64*) obj.data); },
    [](TypeDescriptor*, WriteFormatContext* context) { context->writePrimitive(FormatKey::S64); },
    readNumeric,
    TypeKey::hashEmptyDescriptor,
    TypeKey::alwaysEqualDescriptors,
};

TypeKey TypeKey_U8{
    [](const TypeDescriptor* typeDesc) -> HybridString { return "u8"; },
    [](AnyObject obj, WriteObjectContext* context) { context->out.write<u8>(*(u8*) obj.data); },
    [](TypeDescriptor*, WriteFormatContext* context) { context->writePrimitive(FormatKey::U8); },
    readNumeric,
    TypeKey::hashEmptyDescriptor,
    TypeKey::alwaysEqualDescriptors,
};

TypeKey TypeKey_U16{
    [](const TypeDescriptor* typeDesc) -> HybridString { return "u16"; },
    [](AnyObject obj, WriteObjectContext* context) { context->out.write<u16>(*(u16*) obj.data); },
    [](TypeDescriptor*, WriteFormatContext* context) { context->writePrimitive(FormatKey::U16); },
    readNumeric,
    TypeKey::hashEmptyDescriptor,
    TypeKey::alwaysEqualDescriptors,
};

TypeKey TypeKey_U32{
    [](const TypeDescriptor* typeDesc) -> HybridString { return "u32"; },
    [](AnyObject obj, WriteObjectContext* context) { context->out.write<u32>(*(u32*) obj.data); },
    [](TypeDescriptor*, WriteFormatContext* context) { context->writePrimitive(FormatKey::U32); },
    readNumeric,
    TypeKey::hashEmptyDescriptor,
    TypeKey::alwaysEqualDescriptors,
};

TypeKey TypeKey_U64{
    [](const TypeDescriptor* typeDesc) -> HybridString { return "u64"; },
    [](AnyObject obj, WriteObjectContext* context) { context->out.write<u64>(*(u64*) obj.data); },
    [](TypeDescriptor*, WriteFormatContext* context) { context->writePrimitive(FormatKey::U64); },
    readNumeric,
    TypeKey::hashEmptyDescriptor,
    TypeKey::alwaysEqualDescriptors,
};

TypeKey TypeKey_Float{
    [](const TypeDescriptor* typeDesc) -> HybridString { return "float"; },
    [](AnyObject obj, WriteObjectContext* context) {
        context->out.write<float>(*(float*) obj.data);
    },
    [](TypeDescriptor*, WriteFormatContext* context) { context->writePrimitive(FormatKey::Float); },
    readNumeric,
    TypeKey::hashEmptyDescriptor,
    TypeKey::alwaysEqualDescriptors,
};

TypeKey TypeKey_Double{
    [](const TypeDescriptor* typeDesc) -> HybridString { return "double"; },
    [](AnyObject obj, WriteObjectContext* context) {
        context->out.write<double>(*(double*) obj.data);
    },
    [](TypeDescriptor*, WriteFormatContext* context) {
        context->writePrimitive(FormatKey::Double);
    },
    readNumeric,
    TypeKey::hashEmptyDescriptor,
    TypeKey::alwaysEqualDescriptors,
};

TypeKey TypeKey_String{
    [](const TypeDescriptor* typeDesc) -> HybridString { return "String"; },
    [](AnyObject obj, WriteObjectContext* context) {
        Boxed<String>::write(context->out, *(String*) obj.data);
    },
    [](TypeDescriptor*, WriteFormatContext* context) {
        context->writePrimitive(FormatKey::String);
    },
    [](AnyObject obj, ReadObjectContext* context, FormatDescriptor* formatDesc) {
        PLY_ASSERT(obj.type == getTypeDescriptor<String>());
        if ((FormatKey) formatDesc->formatKey == FormatKey::String) {
            *(String*) obj.data = Boxed<String>::read(context->in);
        } else {
            SLOG(Load, "Can't convert to string");
            skip(context, formatDesc);
        }
    },
    TypeKey::hashEmptyDescriptor,
    TypeKey::alwaysEqualDescriptors,
};

//-----------------------------------------------------------------
// TypeKey_FixedArray
//
TypeKey TypeKey_FixedArray{
    // getName
    [](const TypeDescriptor* typeDesc) -> HybridString { //
        const TypeDescriptor_FixedArray* fixedArrayType =
            typeDesc->cast<const TypeDescriptor_FixedArray>();
        return String::format("FixedArray<{}>", fixedArrayType->itemType->getName());
    },

    // write
    [](AnyObject obj, WriteObjectContext* context) {
        TypeDescriptor_FixedArray* fixedArrayType = obj.type->cast<TypeDescriptor_FixedArray>();
        TypeDescriptor* itemType = fixedArrayType->itemType;
        u32 itemSize = itemType->fixedSize;
        void* item = obj.data;
        for (u32 i : range(fixedArrayType->numItems)) {
            PLY_UNUSED(i);
            itemType->typeKey->write(AnyObject{item, itemType}, context);
            item = PLY_PTR_OFFSET(item, itemSize);
        }
    },
    // writeFormat
    [](TypeDescriptor* typeDesc, WriteFormatContext* context) {
        TypeDescriptor_FixedArray* fixedArrayType = typeDesc->cast<TypeDescriptor_FixedArray>();
        context->writeFixedArray(fixedArrayType->numItems, fixedArrayType->itemType);
    },
    // read
    [](AnyObject obj, ReadObjectContext* context, FormatDescriptor* formatDesc) {
        if ((FormatKey) formatDesc->formatKey != FormatKey::FixedArray) {
            // FIXME: More detailed message
            SLOG(Load, "Can't convert to FixedArray");
            skip(context, formatDesc);
            return;
        }
        FormatDescriptor_FixedArray* fixedFormat = (FormatDescriptor_FixedArray*) formatDesc;
        TypeDescriptor_FixedArray* fixedArrayType = obj.type->cast<TypeDescriptor_FixedArray>();
        if (fixedFormat->numItems != fixedArrayType->numItems) {
            // FIXME: Could probably handle this better
            SLOG(Load, "Fixed array size mismatch");
            skip(context, formatDesc);
            return;
        }
        FormatDescriptor* itemFormat = fixedFormat->itemFormat;
        TypeDescriptor* itemType = fixedArrayType->itemType;
        u32 itemSize = itemType->fixedSize;
        void* item = obj.data;
        for (u32 i : range(fixedArrayType->numItems)) {
            PLY_UNUSED(i);
            itemType->typeKey->read(AnyObject{item, itemType}, context, itemFormat);
            item = PLY_PTR_OFFSET(item, itemSize);
        }
    },
    // hashDescriptor
    [](Hasher& hasher, const TypeDescriptor* typeDesc) {
        const auto* fixedArrayType = typeDesc->cast<const TypeDescriptor_FixedArray>();
        hasher << fixedArrayType->itemType;
        hasher << fixedArrayType->numItems;
        hasher << fixedArrayType->stride;
    },
    // equalDescriptors
    [](const TypeDescriptor* type0, const TypeDescriptor* type1) -> bool {
        const auto* fixedArrayType0 = type0->cast<const TypeDescriptor_FixedArray>();
        const auto* fixedArrayType1 = type1->cast<const TypeDescriptor_FixedArray>();
        return (fixedArrayType0->itemType == fixedArrayType1->itemType) &&
               (fixedArrayType0->numItems == fixedArrayType1->numItems) &&
               (fixedArrayType0->stride == fixedArrayType1->stride);
    },
};

TypeKey* TypeDescriptor_FixedArray::typeKey = &TypeKey_FixedArray;

//-----------------------------------------------------------------
// TypeKey_Array
//
TypeKey TypeKey_Array{
    // getName
    [](const TypeDescriptor* typeDesc) -> HybridString { //
        const TypeDescriptor_Array* arrayType = typeDesc->cast<const TypeDescriptor_Array>();
        return String::format("Array<{}>", arrayType->itemType->getName());
    },

    // write
    [](AnyObject obj, WriteObjectContext* context) {
        TypeDescriptor_Array* arrayType = obj.type->cast<TypeDescriptor_Array>();
        TypeDescriptor* itemType = arrayType->itemType;
        u32 itemSize = itemType->fixedSize;
        impl::BaseArray* arr = (impl::BaseArray*) obj.data;
        void* item = arr->m_items;
        PLY_ASSERT(arr->m_numItems <= UINT32_MAX);
        context->out.write<u32>((u32) arr->m_numItems);
        for (u32 i : range(arr->m_numItems)) {
            PLY_UNUSED(i);
            itemType->typeKey->write(AnyObject{item, itemType}, context);
            item = PLY_PTR_OFFSET(item, itemSize);
        }
    },
    // writeFormat
    [](TypeDescriptor* typeDesc, WriteFormatContext* context) {
        TypeDescriptor_Array* arrayType = typeDesc->cast<TypeDescriptor_Array>();
        context->writeArray(arrayType->itemType);
    },
    // read
    [](AnyObject obj, ReadObjectContext* context, FormatDescriptor* formatDesc) {
        if ((FormatKey) formatDesc->formatKey != FormatKey::Array) {
            // FIXME: More detailed message
            // FIXME: Could probably accept FixedArrays
            SLOG(Load, "Can't convert to Array");
            skip(context, formatDesc);
            return;
        }
        FormatDescriptor_Array* arrayFormat = (FormatDescriptor_Array*) formatDesc;
        FormatDescriptor* itemFormat = arrayFormat->itemFormat;
        TypeDescriptor_Array* arrayType = obj.type->cast<TypeDescriptor_Array>();
        TypeDescriptor* itemType = arrayType->itemType;
        u32 itemSize = itemType->fixedSize;
        u32 arrSize = context->in.read<u32>();
        PLY_ASSERT(arrSize < 10000000);
        impl::BaseArray* arr = (impl::BaseArray*) obj.data;
        // FIXME: Destruct existing elements if array not empty
        arr->realloc(arrSize, itemSize);
        void* item = arr->m_items;
        for (u32 i : range((u32) arrSize)) {
            PLY_UNUSED(i);
            AnyObject typedItem{item, itemType};
            itemType->bindings.construct(typedItem);
            itemType->typeKey->read(typedItem, context, itemFormat);
            item = PLY_PTR_OFFSET(item, itemSize);
        }
    },
    // hashDescriptor
    [](Hasher& hasher, const TypeDescriptor* typeDesc) {
        const auto* arrayType = typeDesc->cast<const TypeDescriptor_Array>();
        hasher << arrayType->itemType;
    },
    // equalDescriptors
    [](const TypeDescriptor* type0, const TypeDescriptor* type1) -> bool {
        const auto* arrayType0 = type0->cast<const TypeDescriptor_Array>();
        const auto* arrayType1 = type1->cast<const TypeDescriptor_Array>();
        return arrayType0->itemType == arrayType1->itemType;
    },
};

TypeKey* TypeDescriptor_Array::typeKey = &TypeKey_Array;

//-----------------------------------------------------------------
// TypeKey_Owned
//
TypeKey TypeKey_Owned{
    // getName
    [](const TypeDescriptor* typeDesc) -> HybridString { //
        const TypeDescriptor_Owned* ownedType = typeDesc->cast<const TypeDescriptor_Owned>();
        return String::format("Owned<{}>", ownedType->targetType->getName());
    },

    // write
    [](AnyObject obj, WriteObjectContext* context) {
        TypeDescriptor_Owned* ownedType = obj.type->cast<TypeDescriptor_Owned>();
        AnyObject targetObj = AnyObject{*(void**) obj.data, ownedType->targetType};

        // Save info needed to resolve weak pointers to this object
        u32 savedOwnedPtrIdx = context->ptrResolver.savedOwnedPtrs.numItems();
        SavedPtrResolver::SavedOwnedPtr& savedInfo = context->ptrResolver.savedOwnedPtrs.append();
        savedInfo.obj = targetObj;
        savedInfo.fileOffset = safeDemote<u32>(context->out.out.get_seek_pos());
        auto cursor = context->ptrResolver.addrToSaveInfo.insertOrFind(
            targetObj.data, &context->ptrResolver.savedOwnedPtrs);
        PLY_ASSERT(!cursor.wasFound());
        *cursor = savedOwnedPtrIdx;

        // Serialize the owned object
        targetObj.type->typeKey->write(targetObj, context);
    },
    // writeFormat
    [](TypeDescriptor* typeDesc, WriteFormatContext* context) {
        TypeDescriptor_Owned* ownedType = typeDesc->cast<TypeDescriptor_Owned>();
        context->writeOwned(ownedType->targetType);
    },
    // read
    [](AnyObject obj, ReadObjectContext* context, FormatDescriptor* formatDesc) {
        if ((FormatKey) formatDesc->formatKey != FormatKey::Owned) {
            // FIXME: More detailed message
            // FIXME: Could probably accept non-owned as long as the format is compatible with
            // child's format
            SLOG(Load, "Can't convert to Owned");
            skip(context, formatDesc);
            return;
        }
        FormatDescriptor_Owned* ownedFormat = (FormatDescriptor_Owned*) formatDesc;
        TypeDescriptor_Owned* ownedType = obj.type->cast<TypeDescriptor_Owned>();
        AnyObject targetObj = AnyObject::create(ownedType->targetType);

        // Save info needed to resolve weak pointers to this object
        while (context->ptrResolver.linkTableIndex < context->ptrResolver.linkTable.numItems()) {
            LoadPtrResolver::LinkTableEntry& linkEntry =
                context->ptrResolver.linkTable[context->ptrResolver.linkTableIndex];
            u32 seekPos =
                safeDemote<u32>(context->in.in.get_seek_pos()) - context->ptrResolver.objDataOffset;
            if (linkEntry.fileOffset > seekPos)
                break; // No weak pointers link here

            context->ptrResolver.linkTableIndex++;
            if (linkEntry.fileOffset == seekPos) {
                linkEntry.ptr = targetObj.data;
#if PLY_VALIDATE_RESOLVED_PTR_TYPES
                linkEntry.typeDesc = targetObj.type;
#endif
                break;
            } else {
                // Object was likely skipped
                linkEntry.ptr = nullptr;
            }
        }

        targetObj.type->typeKey->read(targetObj, context, ownedFormat->childFormat);
        AnyObject ownedSrcPtr = {&targetObj.data, obj.type};
        obj.move(ownedSrcPtr);
        // Should have been moved so there's no need to destruct:
        PLY_ASSERT(targetObj.data == nullptr);
    },
    // hashDescriptor
    [](Hasher& hasher, const TypeDescriptor* typeDesc) {
        const auto* ownedType = typeDesc->cast<const TypeDescriptor_Owned>();
        hasher << ownedType->targetType;
    },
    // equalDescriptors
    [](const TypeDescriptor* type0, const TypeDescriptor* type1) -> bool {
        const auto* ownedType0 = type0->cast<const TypeDescriptor_Owned>();
        const auto* ownedType1 = type1->cast<const TypeDescriptor_Owned>();
        return ownedType0->targetType == ownedType1->targetType;
    },
};

TypeKey* TypeDescriptor_Owned::typeKey = &TypeKey_Owned;

//-----------------------------------------------------------------
// TypeKey_Reference
//
TypeKey TypeKey_Reference{
    // getName
    [](const TypeDescriptor* typeDesc) -> HybridString { //
        const TypeDescriptor_Reference* referencedType =
            typeDesc->cast<const TypeDescriptor_Reference>();
        return String::format("Reference<{}>", referencedType->targetType->getName());
    },

    // write
    [](AnyObject obj, WriteObjectContext* context) {
        TypeDescriptor_Reference* referencedType = obj.type->cast<TypeDescriptor_Reference>();
        AnyObject targetObj = AnyObject{*(void**) obj.data, referencedType->targetType};
        PLY_UNUSED(targetObj);
        PLY_FORCE_CRASH(); // FIXME
    },
    // writeFormat
    [](TypeDescriptor* typeDesc, WriteFormatContext* context) {
        TypeDescriptor_Reference* referencedType = typeDesc->cast<TypeDescriptor_Reference>();
        PLY_UNUSED(referencedType);
        PLY_FORCE_CRASH(); // FIXME
    },
    // read
    [](AnyObject obj, ReadObjectContext* context, FormatDescriptor* formatDesc) {
        PLY_FORCE_CRASH(); // FIXME
    },
    // hashDescriptor
    [](Hasher& hasher, const TypeDescriptor* typeDesc) {
        const auto* referencedType = typeDesc->cast<const TypeDescriptor_Reference>();
        hasher << referencedType->targetType;
    },
    // equalDescriptors
    [](const TypeDescriptor* type0, const TypeDescriptor* type1) -> bool {
        const auto* referencedType0 = type0->cast<const TypeDescriptor_Reference>();
        const auto* referencedType1 = type1->cast<const TypeDescriptor_Reference>();
        return referencedType0->targetType == referencedType1->targetType;
    },
};

TypeKey* TypeDescriptor_Reference::typeKey = &TypeKey_Reference;

//-----------------------------------------------------------------
// TypeKey_Struct
//
TypeDescriptor_Struct::Member* findMember(TypeDescriptor_Struct* structType, const String& name) {
    // FIXME: Improve TypeDescriptor_Struct with lookup table
    for (TypeDescriptor_Struct::Member& member : structType->members) {
        // FIXME: Avoid implicit conversion of member.name (char *) to String
        if (name == member.name)
            return &member;
    }
    return nullptr;
}

TypeKey TypeKey_Struct{
    // getName
    [](const TypeDescriptor* typeDesc) -> HybridString { //
        const TypeDescriptor_Struct* structType = typeDesc->cast<const TypeDescriptor_Struct>();
        return structType->name;
    },

    // write
    [](AnyObject obj, WriteObjectContext* context) {
        TypeDescriptor_Struct* structType = obj.type->cast<TypeDescriptor_Struct>();
        for (const TypeDescriptor_Struct::Member& member : structType->members) {
            AnyObject typedMember{PLY_PTR_OFFSET(obj.data, member.offset), member.type};
            member.type->typeKey->write(typedMember, context);
        }
    },
    // writeFormat
    [](TypeDescriptor* typeDesc, WriteFormatContext* context) {
        TypeDescriptor_Struct* structType = typeDesc->cast<TypeDescriptor_Struct>();
        context->beginStruct(structType->name, structType->templateParams.numItems(),
                             structType->members.numItems());
        for (const TypeDescriptor_Struct::TemplateParam& templateParam :
             structType->templateParams) {
            context->writeTemplateParam(templateParam.name, templateParam.type);
        }
        for (const TypeDescriptor_Struct::Member& member : structType->members) {
            context->writeMember(member.name, member.type);
        }
        context->endStruct();
    },
    // read
    [](AnyObject obj, ReadObjectContext* context, FormatDescriptor* formatDesc) {
        if ((FormatKey) formatDesc->formatKey != FormatKey::Struct) {
            SLOG(Load, "Can't convert to Struct");
            skip(context, formatDesc);
            return;
        }
        FormatDescriptor_Struct* structFormat = (FormatDescriptor_Struct*) formatDesc;
        TypeDescriptor_Struct* structType = obj.type->cast<TypeDescriptor_Struct>();
        for (const FormatDescriptor_Struct::Member& member : structFormat->members) {
            // FIXME: Improve this by finding all the matching struct members ahead of time,
            // and caching them, in a separate pass
            TypeDescriptor_Struct::Member* dstMember = findMember(structType, member.name);
            if (!dstMember) {
                SLOG(Load, "Can't find member \"{}\"", member.name);
                skip(context, member.formatDesc);
                continue;
            }
            AnyObject typedMember{PLY_PTR_OFFSET(obj.data, dstMember->offset), dstMember->type};
            dstMember->type->typeKey->read(typedMember, context, member.formatDesc);
        }
        // FIXME: Identify any members of the structType that *weren't* serialized.
        structType->onPostSerialize(obj.data);
    },
    // hashDescriptor
    [](Hasher& hasher, const TypeDescriptor* typeDesc) {
        const auto* structType = typeDesc->cast<const TypeDescriptor_Struct>();
        hasher << structType->members.numItems();
        for (const auto& member : structType->members) {
            hasher << member.name.view();
            hasher << member.offset;
            hasher << member.type;
        }
    },
    // equalDescriptors
    [](const TypeDescriptor* type0, const TypeDescriptor* type1) -> bool {
        const auto* structType0 = type0->cast<const TypeDescriptor_Struct>();
        const auto* structType1 = type1->cast<const TypeDescriptor_Struct>();
        if (structType0->members.numItems() != structType1->members.numItems())
            return false;
        for (u32 i = 0; i < structType0->members.numItems(); i++) {
            const TypeDescriptor_Struct::Member& member0 = structType0->members[i];
            const TypeDescriptor_Struct::Member& member1 = structType1->members[i];
            if ((member0.name != member1.name) || (member0.offset != member1.offset) ||
                (member0.type != member1.type))
                return false;
        }
        return true;
    },
};

TypeKey* TypeDescriptor_Struct::typeKey = &TypeKey_Struct;

//-----------------------------------------------------------------
// TypeKey_Enum
//
TypeKey TypeKey_Enum{
    // getName
    [](const TypeDescriptor* typeDesc) -> HybridString { //
        const TypeDescriptor_Enum* enumType = typeDesc->cast<const TypeDescriptor_Enum>();
        return enumType->name;
    },

    // write
    [](AnyObject obj, WriteObjectContext* context) {
        TypeDescriptor_Enum* enumType = obj.type->cast<TypeDescriptor_Enum>();
        u32 runTimeValue = 0;
        if (enumType->fixedSize == 1) {
            runTimeValue = *(u8*) obj.data;
        } else if (enumType->fixedSize == 2) {
            runTimeValue = *(u16*) obj.data;
        } else if (enumType->fixedSize == 4) {
            runTimeValue = *(u32*) obj.data;
        } else {
            PLY_ASSERT(0);
        }
        u32 serializedValue = 0;
        // FIXME: Optimize this using a hash table or hybrid array or something:
        for (u32 i = 0; i < enumType->identifiers.numItems(); i++) {
            const TypeDescriptor_Enum::Identifier& identifier = enumType->identifiers[i];
            if (identifier.value == runTimeValue) {
                serializedValue = (u32) i;
                goto found;
            }
        }
        PLY_ASSERT(0); // Value not found; unable to serialize
    found:
        if (enumType->fixedSize == 1) {
            PLY_ASSERT(serializedValue <= UINT8_MAX);
            context->out.write<u8>((u8) serializedValue);
        } else if (enumType->fixedSize == 2) {
            PLY_ASSERT(serializedValue <= UINT16_MAX);
            context->out.write<u16>((u16) serializedValue);
        } else if (enumType->fixedSize == 4) {
            context->out.write<u32>(serializedValue);
        } else {
            PLY_ASSERT(0);
        }
    },
    // writeFormat
    [](TypeDescriptor* typeDesc, WriteFormatContext* context) {
        TypeDescriptor_Enum* enumType = typeDesc->cast<TypeDescriptor_Enum>();
        context->beginEnum(enumType->name, enumType->identifiers.numItems(), enumType->fixedSize);
        for (const TypeDescriptor_Enum::Identifier& identifier : enumType->identifiers) {
            context->writeEnumEntry(identifier.name);
        }
        context->endEnum();
    },
    // read
    [](AnyObject obj, ReadObjectContext* context, FormatDescriptor* formatDesc) {
        NativeEndianReader& in = context->in;
        FormatDescriptor_Enum* enumFormat = (FormatDescriptor_Enum*) formatDesc;
        TypeDescriptor_Enum* enumType = obj.type->cast<TypeDescriptor_Enum>();
        u32 serializedValue = 0;
        if (enumFormat->fixedSize == 1) {
            serializedValue = in.read<u8>();
        } else if (enumFormat->fixedSize == 2) {
            serializedValue = in.read<u16>();
        } else if (enumFormat->fixedSize == 4) {
            serializedValue = in.read<u32>();
        } else {
            PLY_ASSERT(0);
        }
        if (serializedValue >= enumFormat->identifiers.numItems()) {
            SLOG(Load, "Serialized enum value out of range reading \"{}\"", enumType->name);
            // FIXME: In general, it's probably a bad idea to leave the enum value
            // uninitialized...
            return;
        }
        const String& entryName = enumFormat->identifiers[serializedValue];
        u32 runTimeValue = 0;
        // FIXME: Improve this by building a conversion table ahead of time, in a separate pass
        for (const TypeDescriptor_Enum::Identifier& identifier : enumType->identifiers) {
            if (identifier.name == entryName) {
                runTimeValue = identifier.value;
                goto found;
            }
        }
        // Not found
        SLOG(Load, "Can't find enum value \"{}::{}\"", enumType->name, entryName);
        return;
    found:
        if (enumType->fixedSize == 1) {
            PLY_ASSERT(runTimeValue <= UINT8_MAX);
            *(u8*) obj.data = (u8) runTimeValue;
        } else if (enumType->fixedSize == 2) {
            PLY_ASSERT(runTimeValue <= UINT16_MAX);
            *(u16*) obj.data = (u16) runTimeValue;
        } else if (enumType->fixedSize == 4) {
            PLY_ASSERT(runTimeValue <= UINT32_MAX);
            *(u32*) obj.data = (u32) runTimeValue;
        } else {
            PLY_ASSERT(0);
        }
    },
    // hashDescriptor
    nullptr, // Unimplemented
    // equalDescriptors
    nullptr // Unimplemented
};

TypeKey* TypeDescriptor_Enum::typeKey = &TypeKey_Enum;

//-----------------------------------------------------------------
// TypeKey_RawPtr
//
TypeKey TypeKey_RawPtr{
    // getName
    [](const TypeDescriptor* typeDesc) -> HybridString { //
        const TypeDescriptor_RawPtr* weakPtrType = typeDesc->cast<const TypeDescriptor_RawPtr>();
        return String::format("{}*", weakPtrType->getName());
    },

    // write
    [](AnyObject obj, WriteObjectContext* context) {
        TypeDescriptor_RawPtr* weakPtrType = obj.type->cast<TypeDescriptor_RawPtr>();
        AnyObject targetObj = AnyObject{*(void**) obj.data, weakPtrType->targetType};

        // Save info needed to resolve this weak pointer
        SavedPtrResolver::WeakPointerToResolve& weakInfo =
            context->ptrResolver.weakPtrsToResolve.append();
        weakInfo.obj = targetObj;
        weakInfo.fileOffset = safeDemote<u32>(context->out.out.get_seek_pos());

        // Serialize temporary placeholder value
        context->out.write<u32>(0);
    },
    // writeFormat
    [](TypeDescriptor* typeDesc, WriteFormatContext* context) {
        TypeDescriptor_RawPtr* weakPtrType = typeDesc->cast<TypeDescriptor_RawPtr>();
        context->writeRawPtr(weakPtrType->targetType);
    },
    // read
    [](AnyObject obj, ReadObjectContext* context, FormatDescriptor* formatDesc) {
        TypeDescriptor_RawPtr* weakPtrType = obj.type->cast<TypeDescriptor_RawPtr>();
        LoadPtrResolver::RawPtr* weakPtr = (LoadPtrResolver::RawPtr*) obj.data;

        u32 linkIndex = context->in.read<u32>();
        // FIXME: Handle bad data gracefully
        PLY_ASSERT(linkIndex < context->ptrResolver.linkTable.numItems());
        weakPtr->linkIndex = linkIndex;
        LoadPtrResolver::RawPtrToResolve& weakInfo =
            context->ptrResolver.weakPtrsToResolve.append();
        weakInfo.weakPtr = weakPtr;
#if PLY_VALIDATE_RESOLVED_PTR_TYPES
        weakInfo.typeDesc = weakPtrType->targetType;
#endif
    },
    // hashDescriptor
    [](Hasher& hasher, const TypeDescriptor* typeDesc) {
        const auto* weakPtrType = typeDesc->cast<const TypeDescriptor_RawPtr>();
        hasher << weakPtrType->targetType;
    },
    // equalDescriptors
    [](const TypeDescriptor* type0, const TypeDescriptor* type1) -> bool {
        const auto* weakPtrType0 = type0->cast<const TypeDescriptor_RawPtr>();
        const auto* weakPtrType1 = type1->cast<const TypeDescriptor_RawPtr>();
        return weakPtrType0->targetType == weakPtrType1->targetType;
    },
};

TypeKey* TypeDescriptor_RawPtr::typeKey = &TypeKey_RawPtr;

//-----------------------------------------------------------------
// TypeKey_Switch
//
TypeKey TypeKey_Switch{
    // getName
    [](const TypeDescriptor* typeDesc) -> HybridString { //
        const TypeDescriptor_Switch* switchType = typeDesc->cast<const TypeDescriptor_Switch>();
        return switchType->name;
    },
    // write
    [](AnyObject obj, WriteObjectContext* context) {
        TypeDescriptor_Switch* switchType = obj.type->cast<TypeDescriptor_Switch>();
        u16 id = *(u16*) obj.data;
        context->out.write<u16>(id);
        AnyObject typedState{PLY_PTR_OFFSET(obj.data, switchType->storageOffset),
                             switchType->states[id].structType};
        typedState.type->typeKey->write(typedState, context);
    },
    // writeFormat
    [](TypeDescriptor* typeDesc, WriteFormatContext* context) {
        TypeDescriptor_Switch* switchType = typeDesc->cast<TypeDescriptor_Switch>();
        context->beginSwitch(switchType->name, switchType->states.numItems());
        for (const TypeDescriptor_Switch::State& state : switchType->states) {
            context->writeState(state.name, state.structType);
        }
        context->endSwitch();
    },
    // read
    [](AnyObject obj, ReadObjectContext* context, FormatDescriptor* formatDesc) {
        TypeDescriptor_Switch* switchType = obj.type->cast<TypeDescriptor_Switch>();
        if ((FormatKey) formatDesc->formatKey != FormatKey::Switch) {
            SLOG(Load, "Can't convert to switch \"{}\"", switchType->name);
            skip(context, formatDesc);
            return;
        }
        FormatDescriptor_Switch* switchFormat = (FormatDescriptor_Switch*) formatDesc;
        u16 formatStateID = context->in.read<u16>();
        const String& stateName = switchFormat->states[formatStateID].name;
        // FIXME: Improve this by building a conversion table ahead of time, in a separate pass
        u32 newID = 0;
        for (; newID < switchType->states.numItems(); newID++) {
            if (switchType->states[newID].name == stateName)
                break;
        }
        if (newID >= switchType->states.numItems()) {
            SLOG(Load, "Unrecognized state \"{}\" in switch \"{}\"", stateName, switchType->name);
            skip(context, switchFormat->states[formatStateID].structFormat);
            return;
        }
        AnyObject newTypedState{PLY_PTR_OFFSET(obj.data, switchType->storageOffset),
                                switchType->states[newID].structType};
        u16 oldID = *(u16*) obj.data;
        if (oldID != newID) {
            AnyObject oldTypedState{newTypedState.data, switchType->states[oldID].structType};
            oldTypedState.destruct();
            *(u16*) obj.data = newID;
            newTypedState.construct();
        }
        newTypedState.type->typeKey->read(newTypedState, context,
                                          switchFormat->states[newID].structFormat);
    },
    // hashDescriptor
    nullptr, // Unimplemented
    // equalDescriptors
    nullptr // Unimplemented
};

TypeKey* TypeDescriptor_Switch::typeKey = &TypeKey_Switch;

//------------------------------------------------------------------------

void TypeKey::hashEmptyDescriptor(Hasher&, const TypeDescriptor*) {
}

bool TypeKey::alwaysEqualDescriptors(const TypeDescriptor* typeDesc0,
                                     const TypeDescriptor* typeDesc1) {
    PLY_ASSERT(typeDesc0->typeKey == typeDesc1->typeKey);
    return true;
}

} // namespace ply

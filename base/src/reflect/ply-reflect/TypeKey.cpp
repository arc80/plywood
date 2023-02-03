/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-reflect/Core.h>
#include <ply-reflect/TypeKey.h>
#include <ply-reflect/TypeDescriptor.h>
#include <ply-runtime/container/Boxed.h>

namespace ply {

SLOG_CHANNEL(Load, "Load")

//-----------------------------------------------------------------
// Primitive TypeKeys
//
TypeKey TypeKey_Bool{
    [](const TypeDescriptor* typeDesc) -> HybridString { return "bool"; },
    TypeKey::hashEmptyDescriptor,
    TypeKey::alwaysEqualDescriptors,
};

TypeKey TypeKey_S8{
    [](const TypeDescriptor* typeDesc) -> HybridString { return "s8"; },
    TypeKey::hashEmptyDescriptor,
    TypeKey::alwaysEqualDescriptors,
};

TypeKey TypeKey_S16{
    [](const TypeDescriptor* typeDesc) -> HybridString { return "s16"; },
    TypeKey::hashEmptyDescriptor,
    TypeKey::alwaysEqualDescriptors,
};

TypeKey TypeKey_S32{
    [](const TypeDescriptor* typeDesc) -> HybridString { return "s32"; },
    TypeKey::hashEmptyDescriptor,
    TypeKey::alwaysEqualDescriptors,
};

TypeKey TypeKey_S64{
    [](const TypeDescriptor* typeDesc) -> HybridString { return "s64"; },
    TypeKey::hashEmptyDescriptor,
    TypeKey::alwaysEqualDescriptors,
};

TypeKey TypeKey_U8{
    [](const TypeDescriptor* typeDesc) -> HybridString { return "u8"; },
    TypeKey::hashEmptyDescriptor,
    TypeKey::alwaysEqualDescriptors,
};

TypeKey TypeKey_U16{
    [](const TypeDescriptor* typeDesc) -> HybridString { return "u16"; },
    TypeKey::hashEmptyDescriptor,
    TypeKey::alwaysEqualDescriptors,
};

TypeKey TypeKey_U32{
    [](const TypeDescriptor* typeDesc) -> HybridString { return "u32"; },
    TypeKey::hashEmptyDescriptor,
    TypeKey::alwaysEqualDescriptors,
};

TypeKey TypeKey_U64{
    [](const TypeDescriptor* typeDesc) -> HybridString { return "u64"; },
    TypeKey::hashEmptyDescriptor,
    TypeKey::alwaysEqualDescriptors,
};

TypeKey TypeKey_Float{
    [](const TypeDescriptor* typeDesc) -> HybridString { return "float"; },
    TypeKey::hashEmptyDescriptor,
    TypeKey::alwaysEqualDescriptors,
};

TypeKey TypeKey_Double{
    [](const TypeDescriptor* typeDesc) -> HybridString { return "double"; },
    TypeKey::hashEmptyDescriptor,
    TypeKey::alwaysEqualDescriptors,
};

TypeKey TypeKey_String{
    [](const TypeDescriptor* typeDesc) -> HybridString { return "String"; },
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

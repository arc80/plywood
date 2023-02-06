/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
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
    [](const TypeDescriptor* type_desc) -> HybridString { return "bool"; },
    TypeKey::hash_empty_descriptor,
    TypeKey::always_equal_descriptors,
};

TypeKey TypeKey_S8{
    [](const TypeDescriptor* type_desc) -> HybridString { return "s8"; },
    TypeKey::hash_empty_descriptor,
    TypeKey::always_equal_descriptors,
};

TypeKey TypeKey_S16{
    [](const TypeDescriptor* type_desc) -> HybridString { return "s16"; },
    TypeKey::hash_empty_descriptor,
    TypeKey::always_equal_descriptors,
};

TypeKey TypeKey_S32{
    [](const TypeDescriptor* type_desc) -> HybridString { return "s32"; },
    TypeKey::hash_empty_descriptor,
    TypeKey::always_equal_descriptors,
};

TypeKey TypeKey_S64{
    [](const TypeDescriptor* type_desc) -> HybridString { return "s64"; },
    TypeKey::hash_empty_descriptor,
    TypeKey::always_equal_descriptors,
};

TypeKey TypeKey_U8{
    [](const TypeDescriptor* type_desc) -> HybridString { return "u8"; },
    TypeKey::hash_empty_descriptor,
    TypeKey::always_equal_descriptors,
};

TypeKey TypeKey_U16{
    [](const TypeDescriptor* type_desc) -> HybridString { return "u16"; },
    TypeKey::hash_empty_descriptor,
    TypeKey::always_equal_descriptors,
};

TypeKey TypeKey_U32{
    [](const TypeDescriptor* type_desc) -> HybridString { return "u32"; },
    TypeKey::hash_empty_descriptor,
    TypeKey::always_equal_descriptors,
};

TypeKey TypeKey_U64{
    [](const TypeDescriptor* type_desc) -> HybridString { return "u64"; },
    TypeKey::hash_empty_descriptor,
    TypeKey::always_equal_descriptors,
};

TypeKey TypeKey_Float{
    [](const TypeDescriptor* type_desc) -> HybridString { return "float"; },
    TypeKey::hash_empty_descriptor,
    TypeKey::always_equal_descriptors,
};

TypeKey TypeKey_Double{
    [](const TypeDescriptor* type_desc) -> HybridString { return "double"; },
    TypeKey::hash_empty_descriptor,
    TypeKey::always_equal_descriptors,
};

TypeKey TypeKey_String{
    [](const TypeDescriptor* type_desc) -> HybridString { return "String"; },
    TypeKey::hash_empty_descriptor,
    TypeKey::always_equal_descriptors,
};

//-----------------------------------------------------------------
// TypeKey_FixedArray
//
TypeKey TypeKey_FixedArray{
    // get_name
    [](const TypeDescriptor* type_desc) -> HybridString { //
        const TypeDescriptor_FixedArray* fixed_array_type =
            type_desc->cast<const TypeDescriptor_FixedArray>();
        return String::format("FixedArray<{}>",
                              fixed_array_type->item_type->get_name());
    },

    // hash_descriptor
    [](Hasher& hasher, const TypeDescriptor* type_desc) {
        const auto* fixed_array_type =
            type_desc->cast<const TypeDescriptor_FixedArray>();
        hasher << fixed_array_type->item_type;
        hasher << fixed_array_type->num_items;
        hasher << fixed_array_type->stride;
    },
    // equal_descriptors
    [](const TypeDescriptor* type0, const TypeDescriptor* type1) -> bool {
        const auto* fixed_array_type0 = type0->cast<const TypeDescriptor_FixedArray>();
        const auto* fixed_array_type1 = type1->cast<const TypeDescriptor_FixedArray>();
        return (fixed_array_type0->item_type == fixed_array_type1->item_type) &&
               (fixed_array_type0->num_items == fixed_array_type1->num_items) &&
               (fixed_array_type0->stride == fixed_array_type1->stride);
    },
};

TypeKey* TypeDescriptor_FixedArray::type_key = &TypeKey_FixedArray;

//-----------------------------------------------------------------
// TypeKey_Array
//
TypeKey TypeKey_Array{
    // get_name
    [](const TypeDescriptor* type_desc) -> HybridString { //
        const TypeDescriptor_Array* array_type =
            type_desc->cast<const TypeDescriptor_Array>();
        return String::format("Array<{}>", array_type->item_type->get_name());
    },

    // hash_descriptor
    [](Hasher& hasher, const TypeDescriptor* type_desc) {
        const auto* array_type = type_desc->cast<const TypeDescriptor_Array>();
        hasher << array_type->item_type;
    },
    // equal_descriptors
    [](const TypeDescriptor* type0, const TypeDescriptor* type1) -> bool {
        const auto* array_type0 = type0->cast<const TypeDescriptor_Array>();
        const auto* array_type1 = type1->cast<const TypeDescriptor_Array>();
        return array_type0->item_type == array_type1->item_type;
    },
};

TypeKey* TypeDescriptor_Array::type_key = &TypeKey_Array;

//-----------------------------------------------------------------
// TypeKey_Owned
//
TypeKey TypeKey_Owned{
    // get_name
    [](const TypeDescriptor* type_desc) -> HybridString { //
        const TypeDescriptor_Owned* owned_type =
            type_desc->cast<const TypeDescriptor_Owned>();
        return String::format("Owned<{}>", owned_type->target_type->get_name());
    },

    // hash_descriptor
    [](Hasher& hasher, const TypeDescriptor* type_desc) {
        const auto* owned_type = type_desc->cast<const TypeDescriptor_Owned>();
        hasher << owned_type->target_type;
    },
    // equal_descriptors
    [](const TypeDescriptor* type0, const TypeDescriptor* type1) -> bool {
        const auto* owned_type0 = type0->cast<const TypeDescriptor_Owned>();
        const auto* owned_type1 = type1->cast<const TypeDescriptor_Owned>();
        return owned_type0->target_type == owned_type1->target_type;
    },
};

TypeKey* TypeDescriptor_Owned::type_key = &TypeKey_Owned;

//-----------------------------------------------------------------
// TypeKey_Reference
//
TypeKey TypeKey_Reference{
    // get_name
    [](const TypeDescriptor* type_desc) -> HybridString { //
        const TypeDescriptor_Reference* referenced_type =
            type_desc->cast<const TypeDescriptor_Reference>();
        return String::format("Reference<{}>",
                              referenced_type->target_type->get_name());
    },

    // hash_descriptor
    [](Hasher& hasher, const TypeDescriptor* type_desc) {
        const auto* referenced_type = type_desc->cast<const TypeDescriptor_Reference>();
        hasher << referenced_type->target_type;
    },
    // equal_descriptors
    [](const TypeDescriptor* type0, const TypeDescriptor* type1) -> bool {
        const auto* referenced_type0 = type0->cast<const TypeDescriptor_Reference>();
        const auto* referenced_type1 = type1->cast<const TypeDescriptor_Reference>();
        return referenced_type0->target_type == referenced_type1->target_type;
    },
};

TypeKey* TypeDescriptor_Reference::type_key = &TypeKey_Reference;

//-----------------------------------------------------------------
// TypeKey_Struct
//
TypeDescriptor_Struct::Member* find_member(TypeDescriptor_Struct* struct_type,
                                           const String& name) {
    // FIXME: Improve TypeDescriptor_Struct with lookup table
    for (TypeDescriptor_Struct::Member& member : struct_type->members) {
        // FIXME: Avoid implicit conversion of member.name (char *) to String
        if (name == member.name)
            return &member;
    }
    return nullptr;
}

TypeKey TypeKey_Struct{
    // get_name
    [](const TypeDescriptor* type_desc) -> HybridString { //
        const TypeDescriptor_Struct* struct_type =
            type_desc->cast<const TypeDescriptor_Struct>();
        return struct_type->name;
    },

    // hash_descriptor
    [](Hasher& hasher, const TypeDescriptor* type_desc) {
        const auto* struct_type = type_desc->cast<const TypeDescriptor_Struct>();
        hasher << struct_type->members.num_items();
        for (const auto& member : struct_type->members) {
            hasher << member.name.view();
            hasher << member.offset;
            hasher << member.type;
        }
    },
    // equal_descriptors
    [](const TypeDescriptor* type0, const TypeDescriptor* type1) -> bool {
        const auto* struct_type0 = type0->cast<const TypeDescriptor_Struct>();
        const auto* struct_type1 = type1->cast<const TypeDescriptor_Struct>();
        if (struct_type0->members.num_items() != struct_type1->members.num_items())
            return false;
        for (u32 i = 0; i < struct_type0->members.num_items(); i++) {
            const TypeDescriptor_Struct::Member& member0 = struct_type0->members[i];
            const TypeDescriptor_Struct::Member& member1 = struct_type1->members[i];
            if ((member0.name != member1.name) || (member0.offset != member1.offset) ||
                (member0.type != member1.type))
                return false;
        }
        return true;
    },
};

TypeKey* TypeDescriptor_Struct::type_key = &TypeKey_Struct;

//-----------------------------------------------------------------
// TypeKey_Enum
//
TypeKey TypeKey_Enum{
    // get_name
    [](const TypeDescriptor* type_desc) -> HybridString { //
        const TypeDescriptor_Enum* enum_type =
            type_desc->cast<const TypeDescriptor_Enum>();
        return enum_type->name;
    },

    // hash_descriptor
    nullptr, // Unimplemented
    // equal_descriptors
    nullptr // Unimplemented
};

TypeKey* TypeDescriptor_Enum::type_key = &TypeKey_Enum;

//-----------------------------------------------------------------
// TypeKey_RawPtr
//
TypeKey TypeKey_RawPtr{
    // get_name
    [](const TypeDescriptor* type_desc) -> HybridString { //
        const TypeDescriptor_RawPtr* weak_ptr_type =
            type_desc->cast<const TypeDescriptor_RawPtr>();
        return String::format("{}*", weak_ptr_type->get_name());
    },

    // hash_descriptor
    [](Hasher& hasher, const TypeDescriptor* type_desc) {
        const auto* weak_ptr_type = type_desc->cast<const TypeDescriptor_RawPtr>();
        hasher << weak_ptr_type->target_type;
    },
    // equal_descriptors
    [](const TypeDescriptor* type0, const TypeDescriptor* type1) -> bool {
        const auto* weak_ptr_type0 = type0->cast<const TypeDescriptor_RawPtr>();
        const auto* weak_ptr_type1 = type1->cast<const TypeDescriptor_RawPtr>();
        return weak_ptr_type0->target_type == weak_ptr_type1->target_type;
    },
};

TypeKey* TypeDescriptor_RawPtr::type_key = &TypeKey_RawPtr;

//-----------------------------------------------------------------
// TypeKey_Switch
//
TypeKey TypeKey_Switch{
    // get_name
    [](const TypeDescriptor* type_desc) -> HybridString { //
        const TypeDescriptor_Switch* switch_type =
            type_desc->cast<const TypeDescriptor_Switch>();
        return switch_type->name;
    },
    // hash_descriptor
    nullptr, // Unimplemented
    // equal_descriptors
    nullptr // Unimplemented
};

TypeKey* TypeDescriptor_Switch::type_key = &TypeKey_Switch;

//------------------------------------------------------------------------

void TypeKey::hash_empty_descriptor(Hasher&, const TypeDescriptor*) {
}

bool TypeKey::always_equal_descriptors(const TypeDescriptor* type_desc0,
                                       const TypeDescriptor* type_desc1) {
    PLY_ASSERT(type_desc0->type_key == type_desc1->type_key);
    return true;
}

} // namespace ply

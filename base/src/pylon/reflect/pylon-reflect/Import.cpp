/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <pylon-reflect/Core.h>
#include <pylon-reflect/Import.h>
#include <ply-reflect/TypeDescriptorOwner.h>

namespace pylon {

struct PylonTypeImporter {
    TypeDescriptorOwner* type_owner = nullptr;
    const Func<TypeFromName>& type_from_name;

    PylonTypeImporter(const Func<TypeFromName>& type_from_name)
        : type_from_name{type_from_name} {
    }

    TypeDescriptor* convert_type(const Node* a_node) {
        if (a_node->is_text()) {
            // It's a primitive type, represented by a string in Pylon.
            // FIXME: This could use a hash table.
            // Note: TypeKey_SavedTypedPtr/TypeArray::read could use the same hash table
            // if we ever want them to resolve built-in types.
            StringView str = a_node->text();
            if (str == "u16") {
                return get_type_descriptor<u16>();
            } else if (str == "u16_2") {
                return get_type_descriptor<u16[2]>();
            } else if (str == "u16_3") {
                return get_type_descriptor<u16[3]>();
            } else if (str == "u16_4") {
                return get_type_descriptor<u16[4]>();
            } else if (str == "float") {
                return get_type_descriptor<float>();
            } else {
                TypeDescriptor* type_desc = nullptr;
                if (type_from_name) {
                    type_desc = type_from_name(str);
                }
                PLY_ASSERT(type_desc); // Unrecognized primitive type
                return type_desc;
            }
        } else if (a_node->is_object()) {
            // It's either a struct or an enum (not supported yet).
            StringView key = a_node->get("key")->text();
            if (key == "struct") {
                PLY_ASSERT(type_owner); // Must provide an owner for synthesized structs
                // Synthesize a struct
                const Node* a_name = a_node->get("name");
                PLY_ASSERT(a_name->is_text());
                TypeDescriptor_Struct* struct_type =
                    new TypeDescriptor_Struct{0, 0, a_name->text()};
                auto append_member = [&](StringView member_name,
                                         TypeDescriptor* member_type) {
                    struct_type->append_member(member_name, member_type);

                    // FIXME: Different structs will have different alignment
                    // requirements (eg. uniform buffers have different alignment
                    // requirements from vertex attributes). This code only assumes i_os
                    // vertex attributes:
                    u32 alignment = struct_type->fixed_size % 4;
                    if (alignment > 0) {
                        PLY_ASSERT(alignment == 2); // only case currently handled
                        struct_type->append_member("padding",
                                                   get_type_descriptor<u16>());
                    }
                };
                const Node* a_members = a_node->get("members");
                if (a_members->is_object()) {
                    for (const Node::Object::Item& item : a_members->object().items) {
                        append_member(item.key, convert_type(item.value));
                    }
                } else if (a_members->is_array()) {
                    for (const Node* a_member : a_members->array_view()) {
                        PLY_ASSERT(a_member->is_array());
                        PLY_ASSERT(a_member->array_view().num_items == 2);
                        append_member(a_member->array_view()[0]->text(),
                                      convert_type(a_member->array_view()[1]));
                    }
                } else {
                    PLY_ASSERT(0);
                }
                type_owner->adopt_type(struct_type);
                return struct_type;
            } else {
                PLY_ASSERT(0); // Unrecognized or missing type key
            }
        } else {
            // This Pylon node cannot be converted to a TypeDescriptor
            PLY_ASSERT(0);
        }
        return nullptr;
    }
};

TypeDescriptorOwner* convert_type_from(const Node* a_node,
                                       const Func<TypeFromName>& type_from_name) {
    PylonTypeImporter importer{type_from_name};
    importer.type_owner = new TypeDescriptorOwner;
    importer.type_owner->set_root_type(importer.convert_type(a_node));
    return importer.type_owner;
}

void convert_from(AnyObject obj, const Node* a_node,
                  const Func<TypeFromName>& type_from_name) {
    auto error = [&] {}; // FIXME: Decide where these go

    PLY_ASSERT(a_node->is_valid());
    // FIXME: Handle errors gracefully by logging a message, returning false and marking
    // the cook as failed (instead of asserting).
    if (obj.type->type_key == &TypeKey_Struct) {
        PLY_ASSERT(a_node->is_object());
        auto* struct_desc = obj.type->cast<TypeDescriptor_Struct>();
        for (const TypeDescriptor_Struct::Member& member : struct_desc->members) {
            const Node* a_member = a_node->get(member.name);
            if (a_member->is_valid()) {
                AnyObject m{PLY_PTR_OFFSET(obj.data, member.offset), member.type};
                convert_from(m, a_member, type_from_name);
            }
        }
    } else if (obj.type->type_key == &TypeKey_Float) {
        Tuple<bool, double> pair = a_node->numeric();
        PLY_ASSERT(pair.first);
        *(float*) obj.data = (float) pair.second;
    } else if (obj.type->type_key == &TypeKey_U8) {
        Tuple<bool, double> pair = a_node->numeric();
        PLY_ASSERT(pair.first);
        *(u8*) obj.data = (u8) pair.second;
    } else if (obj.type->type_key == &TypeKey_U16) {
        Tuple<bool, double> pair = a_node->numeric();
        PLY_ASSERT(pair.first);
        *(u16*) obj.data = (u16) pair.second;
    } else if (obj.type->type_key == &TypeKey_Bool) {
        *(bool*) obj.data = a_node->text() == "true";
    } else if (obj.type->type_key == &TypeKey_U32) {
        Tuple<bool, double> pair = a_node->numeric();
        PLY_ASSERT(pair.first);
        *(u32*) obj.data = (u32) pair.second;
    } else if (obj.type->type_key == &TypeKey_S32) {
        Tuple<bool, double> pair = a_node->numeric();
        PLY_ASSERT(pair.first);
        *(s32*) obj.data = (s32) pair.second;
    } else if (obj.type->type_key == &TypeKey_FixedArray) {
        PLY_ASSERT(a_node->is_array());
        auto* fixed_arr_type = obj.type->cast<TypeDescriptor_FixedArray>();
        u32 item_size = fixed_arr_type->item_type->fixed_size;
        for (u32 i = 0; i < fixed_arr_type->num_items; i++) {
            AnyObject elem{PLY_PTR_OFFSET(obj.data, item_size * i),
                           fixed_arr_type->item_type};
            convert_from(elem, a_node->get(i), type_from_name);
        }
    } else if (obj.type->type_key == &TypeKey_String) {
        if (a_node->is_text()) {
            *(String*) obj.data = a_node->text();
        } else {
            error();
        }
    } else if (obj.type->type_key == &TypeKey_Array) {
        PLY_ASSERT(a_node->is_array());
        ArrayView<const Node* const> a_node_arr = a_node->array_view();
        auto* arr_type = static_cast<TypeDescriptor_Array*>(obj.type);
        BaseArray* arr = (BaseArray*) obj.data;
        u32 old_arr_size = arr->num_items;
        u32 new_arr_size = a_node_arr.num_items;
        u32 item_size = arr_type->item_type->fixed_size;
        for (u32 i = new_arr_size; i < old_arr_size; i++) {
            AnyObject{PLY_PTR_OFFSET(arr->items, item_size * i), arr_type->item_type}
                .destruct();
        }
        arr->realloc(new_arr_size, item_size);
        for (u32 i = old_arr_size; i < new_arr_size; i++) {
            AnyObject{PLY_PTR_OFFSET(arr->items, item_size * i), arr_type->item_type}
                .construct();
        }
        for (u32 i = 0; i < new_arr_size; i++) {
            AnyObject elem{PLY_PTR_OFFSET(arr->items, item_size * i),
                           arr_type->item_type};
            convert_from(elem, a_node_arr[i], type_from_name);
        }
    } else if (obj.type->type_key == &TypeKey_Enum) {
        PLY_ASSERT(a_node->is_text());
        auto* enum_desc = obj.type->cast<TypeDescriptor_Enum>();
        bool found = false;
        for (const auto& identifier : enum_desc->identifiers) {
            if (identifier.name == a_node->text()) {
                if (enum_desc->fixed_size == 1) {
                    PLY_ASSERT(identifier.value <= UINT8_MAX);
                    *(u8*) obj.data = (u8) identifier.value;
                } else if (enum_desc->fixed_size == 2) {
                    PLY_ASSERT(identifier.value <= UINT16_MAX);
                    *(u16*) obj.data = (u16) identifier.value;
                } else if (enum_desc->fixed_size == 4) {
                    *(u32*) obj.data = identifier.value;
                } else {
                    PLY_ASSERT(0);
                }
                found = true;
                break;
            }
        }
        PLY_ASSERT(found);
        PLY_UNUSED(found);
    } else if (obj.type->type_key == &TypeKey_Switch) {
        PLY_ASSERT(a_node->is_object());
        auto* switch_desc = obj.type->cast<TypeDescriptor_Switch>();
        PLY_ASSERT(a_node->object().items.num_items() == 1);
        auto iter = a_node->object().items.begin();
        const StringView& state_name = iter->key;
        bool found = false;
        for (u32 i = 0; i < switch_desc->states.num_items(); i++) {
            const TypeDescriptor_Switch::State& state = switch_desc->states[i];
            if (state.name == state_name) {
                switch_desc->ensure_state_is(obj, (u16) i);
                AnyObject m{PLY_PTR_OFFSET(obj.data, switch_desc->storage_offset),
                            state.struct_type};
                convert_from(m, iter->value, type_from_name);
                found = true;
                break;
            }
        }
        PLY_ASSERT(found);
        PLY_UNUSED(found);
    } else if (obj.type->type_key == &TypeKey_Owned) {
        auto* owned_desc = obj.type->cast<TypeDescriptor_Owned>();
        AnyObject created = AnyObject::create(owned_desc->target_type);
        *(void**) obj.data = created.data;
        convert_from(created, a_node, type_from_name);
    } else {
        PLY_ASSERT(0); // Unsupported member type
    }
}

AnyOwnedObject import(TypeDescriptor* type_desc, const Node* a_root,
                      const Func<TypeFromName>& type_from_name) {
    AnyOwnedObject result = AnyObject::create(type_desc);
    convert_from(result, a_root, type_from_name);
    return result;
}

void import_into(AnyObject obj, const Node* a_root,
                 const Func<TypeFromName>& type_from_name) {
    convert_from(obj, a_root, type_from_name);
}

} // namespace pylon

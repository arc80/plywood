/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <pylon-reflect/Core.h>
#include <pylon-reflect/Export.h>

namespace pylon {

PLY_NO_INLINE Owned<pylon::Node> export_obj(AnyObject obj, const FilterFunc& filter) {
    if (filter) {
        if (Owned<Node> result = filter(obj))
            return result;
    }

    if (obj.type->type_key == &TypeKey_Struct) {
        const TypeDescriptor_Struct* struct_type =
            obj.type->cast<TypeDescriptor_Struct>();
        Owned<Node> obj_node = Node::create_object();
        for (const TypeDescriptor_Struct::Member& member : struct_type->members) {
            obj_node->set(
                member.name.view(),
                export_obj({PLY_PTR_OFFSET(obj.data, member.offset), member.type},
                           filter));
        }
        return obj_node;
    } else if (obj.type->type_key == &TypeKey_String) {
        return Node::create_text(((String*) obj.data)->view());
    } else if (obj.type->type_key == &TypeKey_Array) {
        Owned<Node> arr_node = Node::create_array();
        const TypeDescriptor_Array* array_type = obj.type->cast<TypeDescriptor_Array>();
        TypeDescriptor* item_type = array_type->item_type;
        u32 item_size = item_type->fixed_size;
        BaseArray* arr = (BaseArray*) obj.data;
        Array<Owned<Node>>& child_nodes = arr_node->array();
        child_nodes.resize(arr->num_items);
        for (u32 i = 0; i < arr->num_items; i++) {
            child_nodes[i] = export_obj(
                {PLY_PTR_OFFSET(arr->items, item_size * i), item_type}, filter);
        }
        return arr_node;
    } else if (obj.type->type_key == &TypeKey_Switch) {
        const TypeDescriptor_Switch* switch_type =
            obj.type->cast<TypeDescriptor_Switch>();
        u16 id = *(u16*) obj.data;
        Owned<Node> obj_node = Node::create_object();
        AnyObject typed_state{PLY_PTR_OFFSET(obj.data, switch_type->storage_offset),
                              switch_type->states[id].struct_type};
        obj_node->set(switch_type->states[id].name.view(),
                      export_obj(typed_state, filter));
        return obj_node;
    } else if (obj.type->type_key == &TypeKey_Bool) {
        return Node::create_text(*(bool*) obj.data ? "true" : "false");
    } else if (obj.type->type_key == &TypeKey_Enum) {
        const TypeDescriptor_Enum* enum_type = obj.type->cast<TypeDescriptor_Enum>();
        // FIXME: Make this a function
        u32 enum_value = 0;
        if (enum_type->fixed_size == 1) {
            enum_value = *(u8*) obj.data;
        } else if (enum_type->fixed_size == 2) {
            enum_value = *(u16*) obj.data;
        } else if (enum_type->fixed_size == 4) {
            enum_value = *(u32*) obj.data;
        } else {
            PLY_ASSERT(0);
        }
        return Node::create_text(enum_type->find_value(enum_value)->name.view());
    } else if (obj.type->type_key == &TypeKey_Owned) {
        auto* owned_desc = obj.type->cast<TypeDescriptor_Owned>();
        AnyObject child{*(void**) obj.data, owned_desc->target_type};
        return export_obj(child, filter);
    } else {
        PLY_ASSERT(0); // Unsupported
        return Node::create_invalid();
    }
}

} // namespace pylon

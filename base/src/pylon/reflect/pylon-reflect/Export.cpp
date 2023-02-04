/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <pylon-reflect/Core.h>
#include <pylon-reflect/Export.h>

namespace pylon {

PLY_NO_INLINE Owned<pylon::Node> exportObj(AnyObject obj, const FilterFunc& filter) {
    if (filter) {
        if (Owned<Node> result = filter(obj))
            return result;
    }

    if (obj.type->typeKey == &TypeKey_Struct) {
        const TypeDescriptor_Struct* structType = obj.type->cast<TypeDescriptor_Struct>();
        Owned<Node> objNode = Node::createObject();
        for (const TypeDescriptor_Struct::Member& member : structType->members) {
            objNode->set(member.name.view(),
                         exportObj({PLY_PTR_OFFSET(obj.data, member.offset), member.type}, filter));
        }
        return objNode;
    } else if (obj.type->typeKey == &TypeKey_String) {
        return Node::createText(((String*) obj.data)->view());
    } else if (obj.type->typeKey == &TypeKey_Array) {
        Owned<Node> arrNode = Node::createArray();
        const TypeDescriptor_Array* arrayType = obj.type->cast<TypeDescriptor_Array>();
        TypeDescriptor* itemType = arrayType->itemType;
        u32 itemSize = itemType->fixedSize;
        BaseArray* arr = (BaseArray*) obj.data;
        Array<Owned<Node>>& childNodes = arrNode->array();
        childNodes.resize(arr->num_items);
        for (u32 i = 0; i < arr->num_items; i++) {
            childNodes[i] =
                exportObj({PLY_PTR_OFFSET(arr->items, itemSize * i), itemType}, filter);
        }
        return arrNode;
    } else if (obj.type->typeKey == &TypeKey_Switch) {
        const TypeDescriptor_Switch* switchType = obj.type->cast<TypeDescriptor_Switch>();
        u16 id = *(u16*) obj.data;
        Owned<Node> objNode = Node::createObject();
        AnyObject typedState{PLY_PTR_OFFSET(obj.data, switchType->storageOffset),
                             switchType->states[id].structType};
        objNode->set(switchType->states[id].name.view(), exportObj(typedState, filter));
        return objNode;
    } else if (obj.type->typeKey == &TypeKey_Bool) {
        return Node::createText(*(bool*) obj.data ? "true" : "false");
    } else if (obj.type->typeKey == &TypeKey_Enum) {
        const TypeDescriptor_Enum* enumType = obj.type->cast<TypeDescriptor_Enum>();
        // FIXME: Make this a function
        u32 enumValue = 0;
        if (enumType->fixedSize == 1) {
            enumValue = *(u8*) obj.data;
        } else if (enumType->fixedSize == 2) {
            enumValue = *(u16*) obj.data;
        } else if (enumType->fixedSize == 4) {
            enumValue = *(u32*) obj.data;
        } else {
            PLY_ASSERT(0);
        }
        return Node::createText(enumType->findValue(enumValue)->name.view());
    } else if (obj.type->typeKey == &TypeKey_Owned) {
        auto* ownedDesc = obj.type->cast<TypeDescriptor_Owned>();
        AnyObject child{*(void**) obj.data, ownedDesc->targetType};
        return exportObj(child, filter);
    } else {
        PLY_ASSERT(0); // Unsupported
        return Node::createInvalid();
    }
}

} // namespace pylon

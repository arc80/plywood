/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <pylon-reflect/Core.h>
#include <pylon-reflect/Export.h>

namespace pylon {

PLY_NO_INLINE void exportObjTo(Node& aNode, TypedPtr obj, const FilterFunc& filter) {
    if (filter && filter(aNode, obj))
        return;

    if (obj.type->typeKey == &TypeKey_Struct) {
        const TypeDescriptor_Struct* structType = obj.type->cast<TypeDescriptor_Struct>();
        auto objNode = aNode.type.object().switchTo();
        for (const TypeDescriptor_Struct::Member& member : structType->members) {
            Node::Object::Item& objItem = objNode->obj.add(member.name.view());
            exportObjTo(objItem.value, {PLY_PTR_OFFSET(obj.ptr, member.offset), member.type},
                        filter);
        }
    } else if (obj.type->typeKey == &TypeKey_String) {
        auto text = aNode.type.text().switchTo();
        text->str = ((String*) obj.ptr)->view();
    } else if (obj.type->typeKey == &TypeKey_Array) {
        const TypeDescriptor_Array* arrayType = obj.type->cast<TypeDescriptor_Array>();
        TypeDescriptor* itemType = arrayType->itemType;
        u32 itemSize = itemType->fixedSize;
        details::BaseArray* arr = (details::BaseArray*) obj.ptr;
        auto arrNode = aNode.type.array().switchTo();
        arrNode->arr.resize(arr->m_numItems);
        for (u32 i : range(arr->m_numItems)) {
            exportObjTo(arrNode->arr[i], {PLY_PTR_OFFSET(arr->m_items, itemSize * i), itemType},
                        filter);
        }
    } else if (obj.type->typeKey == &TypeKey_Switch) {
        const TypeDescriptor_Switch* switchType = obj.type->cast<TypeDescriptor_Switch>();
        u16 id = *(u16*) obj.ptr;
        auto objNode = aNode.type.object().switchTo();
        Node::Object::Item& objItem = objNode->obj.add(switchType->states[id].name.view());
        TypedPtr typedState{PLY_PTR_OFFSET(obj.ptr, switchType->storageOffset),
                            switchType->states[id].structType};
        exportObjTo(objItem.value, typedState, filter);
    } else if (obj.type->typeKey == &TypeKey_Bool) {
        auto text = aNode.type.text().switchTo();
        text->str = *(bool*) obj.ptr ? "true" : "false";
    } else if (obj.type->typeKey == &TypeKey_Enum) {
        const TypeDescriptor_Enum* enumType = obj.type->cast<TypeDescriptor_Enum>();
        // FIXME: Make this a function
        u32 enumValue = 0;
        if (enumType->fixedSize == 1) {
            enumValue = *(u8*) obj.ptr;
        } else if (enumType->fixedSize == 2) {
            enumValue = *(u16*) obj.ptr;
        } else if (enumType->fixedSize == 4) {
            enumValue = *(u32*) obj.ptr;
        } else {
            PLY_ASSERT(0);
        }
        aNode = pylon::Node::createText(enumType->findValue(enumValue)->name.view(), {});
    } else {
        PLY_ASSERT(0); // Unsupported
    }
}

PLY_NO_INLINE pylon::Node exportObj(TypedPtr obj, const FilterFunc& filter) {
    pylon::Node node;
    exportObjTo(node, obj, filter);
    return node;
}

} // namespace pylon

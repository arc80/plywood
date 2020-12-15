/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <pylon-reflect/Core.h>
#include <pylon-reflect/Export.h>

namespace pylon {

PLY_NO_INLINE Owned<pylon::Node> exportObj(TypedPtr obj, const FilterFunc& filter) {
    if (filter) {
        if (Owned<Node> result = filter(obj))
            return result;
    }

    if (obj.type->typeKey == &TypeKey_Struct) {
        const TypeDescriptor_Struct* structType = obj.type->cast<TypeDescriptor_Struct>();
        Owned<Node> objNode = Node::createObject();
        for (const TypeDescriptor_Struct::Member& member : structType->members) {
            objNode->set(member.name.view(),
                         exportObj({PLY_PTR_OFFSET(obj.ptr, member.offset), member.type}, filter));
        }
        return objNode;
    } else if (obj.type->typeKey == &TypeKey_String) {
        return Node::createText(((String*) obj.ptr)->view());
    } else if (obj.type->typeKey == &TypeKey_Array) {
        Owned<Node> arrNode = Node::createArray();
        const TypeDescriptor_Array* arrayType = obj.type->cast<TypeDescriptor_Array>();
        TypeDescriptor* itemType = arrayType->itemType;
        u32 itemSize = itemType->fixedSize;
        details::BaseArray* arr = (details::BaseArray*) obj.ptr;
        Array<Owned<Node>>& childNodes = arrNode->array();
        childNodes.resize(arr->m_numItems);
        for (u32 i : range(arr->m_numItems)) {
            childNodes[i] =
                exportObj({PLY_PTR_OFFSET(arr->m_items, itemSize * i), itemType}, filter);
        }
        return arrNode;
    } else if (obj.type->typeKey == &TypeKey_Switch) {
        const TypeDescriptor_Switch* switchType = obj.type->cast<TypeDescriptor_Switch>();
        u16 id = *(u16*) obj.ptr;
        Owned<Node> objNode = Node::createObject();
        TypedPtr typedState{PLY_PTR_OFFSET(obj.ptr, switchType->storageOffset),
                            switchType->states[id].structType};
        objNode->set(switchType->states[id].name.view(), exportObj(typedState, filter));
        return objNode;
    } else if (obj.type->typeKey == &TypeKey_Bool) {
        return Node::createText(*(bool*) obj.ptr ? "true" : "false");
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
        return Node::createText(enumType->findValue(enumValue)->name.view());
    } else if (obj.type->typeKey == &TypeKey_Owned) {
        auto* ownedDesc = obj.type->cast<TypeDescriptor_Owned>();
        TypedPtr child{*(void**) obj.ptr, ownedDesc->targetType};
        return exportObj(child, filter);
    } else {
        PLY_ASSERT(0); // Unsupported
        return Node::createInvalid();
    }
}

} // namespace pylon

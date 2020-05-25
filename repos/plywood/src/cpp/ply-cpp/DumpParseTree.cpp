/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-cpp/Core.h>
#include <ply-cpp/Parser.h>
#include <ply-cpp/Grammar.h>

namespace ply {
namespace cpp {

void dumpParseTree(StringWriter* sw, TypedPtr any, u32 indent) {
    auto doIndent = [&]() {
        for (u32 i = 0; i < indent; i++) {
            *sw << "  ";
        }
    };
    indent++;
    TypeKey* typeKey = any.type->typeKey;
    if (typeKey == TypeDescriptor_Struct::typeKey) {
        const TypeDescriptor_Struct* structType = any.type->cast<TypeDescriptor_Struct>();
        if (structType->name == "ply::cpp::Token") {
            const Token* token = (const Token*) any.ptr;
            sw->format("\"{}\"\n", fmt::EscapedString{token->identifier});
        } else if (structType->name == "ply::cpp::grammar::QualifiedID") {
            sw->format("\"{}\"", ((grammar::QualifiedID*) any.ptr)->toString());
            *sw << '\n';
        } else {
            *sw << structType->name << '\n';
            for (const TypeDescriptor_Struct::Member& member : structType->members) {
                doIndent();
                sw->format("{}: ", member.name);
                TypedPtr anyMember{PLY_PTR_OFFSET(any.ptr, member.offset), member.type};
                dumpParseTree(sw, anyMember, indent);
            }
        }
    } else if (typeKey == TypeDescriptor_Enum::typeKey) {
        const TypeDescriptor_Enum* enumType = any.type->cast<TypeDescriptor_Enum>();
        u32 runTimeValue = 0;
        if (enumType->fixedSize == 1) {
            runTimeValue = *(u8*) any.ptr;
        } else if (enumType->fixedSize == 2) {
            runTimeValue = *(u16*) any.ptr;
        } else if (enumType->fixedSize == 4) {
            runTimeValue = *(u32*) any.ptr;
        } else {
            PLY_ASSERT(0);
        }
        sw->format("{}::{}\n", enumType->name, enumType->findValue(runTimeValue)->name);
    } else if (typeKey == TypeDescriptor_Array::typeKey) {
        TypeDescriptor_Array* arrayType = any.type->cast<TypeDescriptor_Array>();
        TypeDescriptor* itemType = arrayType->itemType;
        *sw << "[]\n";
        details::BaseArray* arr = (details::BaseArray*) any.ptr;
        void* item = arr->m_items;
        for (u32 i = 0; i < arr->m_numItems; i++) {
            doIndent();
            sw->format("[{}] ", i);
            dumpParseTree(sw, TypedPtr{item, itemType}, indent);
            item = PLY_PTR_OFFSET(item, itemType->fixedSize);
        }
    } else if (typeKey == TypeDescriptor_Switch::typeKey) {
        TypeDescriptor_Switch* switchType = any.type->cast<TypeDescriptor_Switch>();
        u16 id = *(u16*) any.ptr;
        void* buf = PLY_PTR_OFFSET(any.ptr, switchType->storageOffset);
        dumpParseTree(sw, TypedPtr{buf, switchType->states[id].structType}, indent - 1);
    } else if (typeKey == TypeDescriptor_Owned::typeKey) {
        TypeDescriptor_Owned* ownedType = any.type->cast<TypeDescriptor_Owned>();
        TypedPtr targetPtr = TypedPtr{*(void**) any.ptr, ownedType->targetType};
        if (targetPtr.ptr) {
            dumpParseTree(sw, targetPtr, indent - 1);
        } else {
            *sw << "(null)\n";
        }
    } else {
        PLY_ASSERT(0);
    }
}

} // namespace cpp
} // namespace ply

/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-cpp/Core.h>
#include <ply-cpp/Parser.h>
#include <ply-cpp/Grammar.h>
#include <ply-cpp/ErrorFormatting.h>

namespace ply {
namespace cpp {

void dumpParseTree(OutStream* outs, AnyObject any, u32 indent, const PPVisitedFiles* visitedFiles) {
    auto doIndent = [&]() {
        for (u32 i = 0; i < indent; i++) {
            *outs << "  ";
        }
    };
    indent++;
    TypeKey* typeKey = any.type->typeKey;
    if (typeKey == TypeDescriptor_Struct::typeKey) {
        const TypeDescriptor_Struct* structType = any.type->cast<TypeDescriptor_Struct>();
        if (structType->name == "ply::cpp::Token") {
            const Token* token = (const Token*) any.data;
            if (token->linearLoc >= 0 && visitedFiles) {
                outs->format("{}: ", expandFileLocation(visitedFiles, token->linearLoc).toString());
            }
            outs->format("\"{}\"\n", fmt::EscapedString{token->identifier});
        } else if (structType->name == "ply::cpp::grammar::QualifiedID") {
            outs->format("\"{}\"", ((grammar::QualifiedID*) any.data)->toString());
            *outs << '\n';
        } else {
            *outs << structType->name << '\n';
            for (const TypeDescriptor_Struct::Member& member : structType->members) {
                doIndent();
                outs->format("{}: ", member.name);
                AnyObject anyMember{PLY_PTR_OFFSET(any.data, member.offset), member.type};
                dumpParseTree(outs, anyMember, indent, visitedFiles);
            }
        }
    } else if (typeKey == TypeDescriptor_Enum::typeKey) {
        const TypeDescriptor_Enum* enumType = any.type->cast<TypeDescriptor_Enum>();
        u32 runTimeValue = 0;
        if (enumType->fixedSize == 1) {
            runTimeValue = *(u8*) any.data;
        } else if (enumType->fixedSize == 2) {
            runTimeValue = *(u16*) any.data;
        } else if (enumType->fixedSize == 4) {
            runTimeValue = *(u32*) any.data;
        } else {
            PLY_ASSERT(0);
        }
        outs->format("{}::{}\n", enumType->name, enumType->findValue(runTimeValue)->name);
    } else if (typeKey == TypeDescriptor_Array::typeKey) {
        TypeDescriptor_Array* arrayType = any.type->cast<TypeDescriptor_Array>();
        TypeDescriptor* itemType = arrayType->itemType;
        *outs << "[]\n";
        details::BaseArray* arr = (details::BaseArray*) any.data;
        void* item = arr->m_items;
        for (u32 i = 0; i < arr->m_numItems; i++) {
            doIndent();
            outs->format("[{}] ", i);
            dumpParseTree(outs, AnyObject{item, itemType}, indent, visitedFiles);
            item = PLY_PTR_OFFSET(item, itemType->fixedSize);
        }
    } else if (typeKey == TypeDescriptor_Switch::typeKey) {
        TypeDescriptor_Switch* switchType = any.type->cast<TypeDescriptor_Switch>();
        u16 id = *(u16*) any.data;
        void* buf = PLY_PTR_OFFSET(any.data, switchType->storageOffset);
        dumpParseTree(outs, AnyObject{buf, switchType->states[id].structType}, indent - 1,
                      visitedFiles);
    } else if (typeKey == TypeDescriptor_Owned::typeKey) {
        TypeDescriptor_Owned* ownedType = any.type->cast<TypeDescriptor_Owned>();
        AnyObject targetObj = AnyObject{*(void**) any.data, ownedType->targetType};
        if (targetObj.data) {
            dumpParseTree(outs, targetObj, indent - 1, visitedFiles);
        } else {
            *outs << "(null)\n";
        }
    } else if (typeKey == &TypeKey_Bool) {
        *outs << (*(bool*) any.data ? "true\n" : "false\n");
    } else {
        PLY_ASSERT(0);
    }
}

} // namespace cpp
} // namespace ply

/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-reflect/Core.h>
#include <ply-reflect/TypeKey.h>
#include <ply-reflect/PersistWrite.h>
#include <ply-reflect/FormatDescriptor.h>
#include <ply-reflect/TypedArray.h>
#include <ply-reflect/AnySavedObject.h>
#include <ply-runtime/container/Boxed.h>
#include <map>

namespace ply {

Array<BuiltInPair> BuiltInTypeDescs = {{FormatKey::None, nullptr},
                                       {FormatKey::Indirect, nullptr},
                                       {FormatKey::Bool, getTypeDescriptor<bool>()},
                                       {FormatKey::S8, getTypeDescriptor<s8>()},
                                       {FormatKey::S16, getTypeDescriptor<s16>()},
                                       {FormatKey::S32, getTypeDescriptor<s32>()},
                                       {FormatKey::S64, getTypeDescriptor<s64>()},
                                       {FormatKey::U8, getTypeDescriptor<u8>()},
                                       {FormatKey::U16, getTypeDescriptor<u16>()},
                                       {FormatKey::U32, getTypeDescriptor<u32>()},
                                       {FormatKey::U64, getTypeDescriptor<u64>()},
                                       {FormatKey::Float, getTypeDescriptor<float>()},
                                       {FormatKey::Double, getTypeDescriptor<double>()},
                                       {FormatKey::String, getTypeDescriptor<String>()},
                                       {FormatKey::TypedArray, getTypeDescriptor<TypedArray>()},
                                       {FormatKey::Typed, getTypeDescriptor<AnySavedObject>()}};

u32 WriteFormatContext::addFormatDesc(TypeDescriptor* typeDesc) {
    PLY_ASSERT(m_typeToFormatID.find(typeDesc) == m_typeToFormatID.end());
    u32 formatID = m_nextUserFormatID++;
    m_typeToFormatID[typeDesc] = formatID;
#if PLY_DEBUG_WRITE_FORMAT_CONTEXT
    m_userFormatTypeDescs.append(typeDesc);
#endif
    typeDesc->typeKey->writeFormat(typeDesc, this);
    return formatID;
}

void WriteFormatContext::writeChildFormatDesc(TypeDescriptor* typeDesc) {
    auto iter = m_typeToFormatID.find(typeDesc);
    if (iter == m_typeToFormatID.end()) {
        addFormatDesc(typeDesc);
    } else {
        // Indirect reference to FormatDescriptor (maybe partially written) that was already
        // assigned a formatID.
        m_out.write<u8>((u8) FormatKey::Indirect);
        m_out.write<u32>(iter->second);
    }
}

WriteFormatContext::WriteFormatContext(OutStream* out) : m_out{out} {
    // Initialize m_typeToFormatID with built-in types
    PLY_ASSERT(BuiltInTypeDescs.numItems() == (u32) FormatKey::StartUserKeyRange);
    for (u32 builtinIndex : range(BuiltInTypeDescs.numItems())) {
        const auto& builtin = BuiltInTypeDescs[builtinIndex];
        PLY_ASSERT((u32) builtin.key == builtinIndex);
        if (builtin.type) {
            m_typeToFormatID[builtin.type] = (u32) builtin.key;
        }
    }
}

u32 WriteFormatContext::addOrGetFormatID(TypeDescriptor* typeDesc) {
    auto iter = m_typeToFormatID.find(typeDesc);
    if (iter == m_typeToFormatID.end())
        return addFormatDesc(typeDesc);
    else
        return iter->second;
}

u32 WriteFormatContext::getFormatID(TypeDescriptor* typeDesc) const {
    auto iter = m_typeToFormatID.find(typeDesc);
    PLY_ASSERT(iter != m_typeToFormatID.end());
    return iter->second;
}

void WriteFormatContext::endSchema() {
    // Write an empty FormatDescriptor to terminate the list
    m_out.write<u8>((u8) FormatKey::None);
}

void WriteFormatContext::writePrimitive(FormatKey formatKey) {
    PLY_ASSERT(formatKey > FormatKey::Indirect);
    PLY_ASSERT(formatKey < FormatKey::StartUserKeyRange);
    m_out.write<u8>((u8) formatKey);
}

void WriteFormatContext::writeFixedArray(u32 numItems, TypeDescriptor* itemType) {
    m_out.write<u8>((u8) FormatKey::FixedArray);
    PLY_ASSERT(numItems <= UINT32_MAX);
    m_out.write<u32>((u32) numItems);
    writeChildFormatDesc(itemType);
}

void WriteFormatContext::writeArray(TypeDescriptor* itemType) {
    m_out.write<u8>((u8) FormatKey::Array);
    writeChildFormatDesc(itemType);
}

void WriteFormatContext::writeOwned(TypeDescriptor* targetType) {
    m_out.write<u8>((u8) FormatKey::Owned);
    writeChildFormatDesc(targetType);
}

void WriteFormatContext::writeRawPtr(TypeDescriptor* targetType) {
    m_out.write<u8>((u8) FormatKey::RawPtr);
    writeChildFormatDesc(targetType);
}

// FIXME: Make this function (and others in this file) take a StringRef_UTF8 instead and make
// sure TypeWriter<StringRef_UTF8> works properly
void WriteFormatContext::beginStruct(const String& name, u32 numTemplateParams, u32 numMembers) {
    m_out.write<u8>((u8) FormatKey::Struct);
    Boxed<String>::write(m_out, name);
    PLY_ASSERT(numTemplateParams <= UINT16_MAX);
    m_out.write<u16>((u16) numTemplateParams);
    PLY_ASSERT(numMembers <= UINT16_MAX);
    m_out.write<u16>((u16) numMembers);
}

void WriteFormatContext::writeTemplateParam(const String& name, TypeDescriptor* typeDesc) {
    Boxed<String>::write(m_out, name);
    writeChildFormatDesc(typeDesc);
}

void WriteFormatContext::writeMember(const String& name, TypeDescriptor* typeDesc) {
    Boxed<String>::write(m_out, name);
    writeChildFormatDesc(typeDesc);
}

void WriteFormatContext::endStruct() {
    // FIXME: Verify that the correct number of template params & members were written
}

void WriteFormatContext::beginEnum(const String& name, u32 numEntries, u8 fixedSize) {
    m_out.write<u8>((u8) FormatKey::Enum);
    Boxed<String>::write(m_out, name);
    m_out.write<u8>((u8) fixedSize);
    PLY_ASSERT(numEntries <= UINT32_MAX);
    m_out.write<u32>((u32) numEntries);
}

void WriteFormatContext::writeEnumEntry(const String& name) {
    Boxed<String>::write(m_out, name);
}

void WriteFormatContext::endEnum() {
    // FIXME: Verify that the correct number of entries was written
}

void WriteFormatContext::writeEnumIndexedArray(TypeDescriptor* itemType,
                                               TypeDescriptor_Enum* enumType) {
    m_out.write<u8>((u8) FormatKey::EnumIndexedArray);
    writeChildFormatDesc(itemType);
    writeChildFormatDesc(enumType);
}

void WriteFormatContext::beginSwitch(const String& name, u32 numStates) {
    m_out.write<u8>((u8) FormatKey::Switch);
    Boxed<String>::write(m_out, name);
    PLY_ASSERT(numStates <= UINT16_MAX);
    m_out.write<u16>((u16) numStates);
}

void WriteFormatContext::writeState(const String& name, TypeDescriptor_Struct* structType) {
    Boxed<String>::write(m_out, name);
    writeChildFormatDesc(structType);
}

void WriteFormatContext::endSwitch() {
    // FIXME: Verify that the correct number of states was written
}

} // namespace ply

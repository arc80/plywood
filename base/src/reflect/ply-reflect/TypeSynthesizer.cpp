/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-reflect/Core.h>
#include <ply-reflect/TypeSynthesizer.h>
#include <ply-reflect/FormatDescriptor.h>
#include <ply-reflect/SynthTypeDeduplicator.h>

namespace ply {

// Currently, we just use a global hook for the app to install custom type synthesizers.
// If needed, we could use a more flexible approach in the future:
std::map<String, SynthFunc> g_TypeSynthRegistry;

struct TypeSynthesizer {
    TypeDescriptorOwner* m_typeDescOwner = nullptr;
    std::map<FormatDescriptor*, TypeDescriptor*> m_formatToType;
};

TypeDescriptor* synthesize(TypeSynthesizer* synth, FormatDescriptor* formatDesc) {
    PLY_ASSERT(formatDesc);

    // Handle primitive built-in types
    if (formatDesc->formatKey < (u8) FormatKey::StartUserKeyRange) {
        BuiltInPair& pair = BuiltInTypeDescs[formatDesc->formatKey];
        PLY_ASSERT(pair.type);
        PLY_ASSERT(pair.key == (FormatKey) formatDesc->formatKey);
        return pair.type;
    }

    // See if a TypeDescriptor was already synthesized for this FormatDescriptor
    auto iter = synth->m_formatToType.find(formatDesc);
    if (iter != synth->m_formatToType.end())
        return iter->second;

    // Synthesize a new TypeDescriptor
    TypeDescriptor* typeDesc = nullptr;
    switch ((FormatKey) formatDesc->formatKey) {
        case FormatKey::FixedArray: {
            FormatDescriptor_FixedArray* fixedFormat = (FormatDescriptor_FixedArray*) formatDesc;
            TypeDescriptor* itemType = synthesize(synth, fixedFormat->itemFormat);
            typeDesc = new TypeDescriptor_FixedArray{itemType, fixedFormat->numItems};
            synth->m_typeDescOwner->adoptType(typeDesc);
            break;
        }

        case FormatKey::Array: {
            FormatDescriptor_Array* arrayFormat = (FormatDescriptor_Array*) formatDesc;
            TypeDescriptor* itemType = synthesize(synth, arrayFormat->itemFormat);
            typeDesc = new TypeDescriptor_Array{itemType};
            synth->m_typeDescOwner->adoptType(typeDesc);
            break;
        }

        case FormatKey::Struct: {
            FormatDescriptor_Struct* structFormat = (FormatDescriptor_Struct*) formatDesc;
            auto iter = g_TypeSynthRegistry.find(structFormat->name);
            if (iter != g_TypeSynthRegistry.end()) {
                // Currently, we just use a global hook for the app to install custom type
                // synthesizers. If needed, we could use a more flexible approach in the
                // future:
                typeDesc = iter->second(synth, formatDesc);
            } else {
                // Synthesize a struct
                TypeDescriptor_Struct* structType =
                    new TypeDescriptor_Struct{0, 0, structFormat->name};
                u32 memberOffset = 0;
                u32 alignment = 1;
                for (const FormatDescriptor_Struct::Member& member : structFormat->members) {
                    TypeDescriptor* memberType = synthesize(synth, member.formatDesc);
                    structType->members.append({member.name, memberOffset, memberType});
                    memberOffset += memberType->fixedSize;
                    alignment = max(alignment, memberType->alignment);
                }
                structType->fixedSize = memberOffset;
                structType->alignment = alignment;
                typeDesc = structType;
                synth->m_typeDescOwner->adoptType(typeDesc);
            }
            break;
        }

        default: {
            // Shouldn't get here
            PLY_ASSERT(0);
            break;
        }
    }

    // Add it to owner and lookup table
    PLY_ASSERT(typeDesc);
    synth->m_formatToType[formatDesc] = typeDesc;
    return typeDesc;
}

void append(TypeSynthesizer* synth, TypeDescriptor* typeDesc) {
    synth->m_typeDescOwner->adoptType(typeDesc);
}

Reference<TypeDescriptorOwner> synthesizeType(FormatDescriptor* formatDesc) {
    Reference<TypeDescriptorOwner> typeDescOwner = new TypeDescriptorOwner;
    TypeSynthesizer synth;
    synth.m_typeDescOwner = typeDescOwner;
    typeDescOwner->setRootType(synthesize(&synth, formatDesc));
    return getUniqueType(&g_typeDedup, typeDescOwner);
}

} // namespace ply

/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-reflect/Core.h>
#include <ply-reflect/PersistRead.h>
#include <ply-reflect/FormatDescriptor.h>
#include <ply-runtime/container/Boxed.h>

namespace ply {

struct BuiltInFormatPair {
    FormatKey key;
    FormatDescriptor* formatDesc;
};

Array<BuiltInFormatPair> BuiltInFormats = {{FormatKey::None, nullptr},
                                           {FormatKey::Indirect, nullptr},
                                           {FormatKey::Bool, &FormatDescriptor_Bool},
                                           {FormatKey::S8, &FormatDescriptor_S8},
                                           {FormatKey::S16, &FormatDescriptor_S16},
                                           {FormatKey::S32, &FormatDescriptor_S32},
                                           {FormatKey::S64, &FormatDescriptor_S64},
                                           {FormatKey::U8, &FormatDescriptor_U8},
                                           {FormatKey::U16, &FormatDescriptor_U16},
                                           {FormatKey::U32, &FormatDescriptor_U32},
                                           {FormatKey::U64, &FormatDescriptor_U64},
                                           {FormatKey::Float, &FormatDescriptor_Float},
                                           {FormatKey::Double, &FormatDescriptor_Double},
                                           {FormatKey::String, &FormatDescriptor_String},
                                           {FormatKey::TypedArray, &FormatDescriptor_TypedArray},
                                           {FormatKey::Typed, &FormatDescriptor_Typed}};

class SchemaLoader {
public:
    Schema* m_schema;
    NativeEndianReader m_in;

    SchemaLoader(Schema* schema, InStream* in) : m_schema(schema), m_in(in) {
        PLY_ASSERT(BuiltInFormats.numItems() == int(FormatKey::StartUserKeyRange));
    }

    FormatDescriptor* readFormatDescriptor(bool topLevel = false) {
        u8 formatKey = m_in.read<u8>();
        if (formatKey == (u8) FormatKey::None) {
            PLY_ASSERT(topLevel);
            return nullptr;
        }

        if (formatKey == (u8) FormatKey::Indirect) {
            u32 formatID = m_in.read<u32>();
            if (formatID < FormatID_StartUserRange) {
                PLY_ASSERT(formatID < (u32) FormatKey::StartUserKeyRange);
                BuiltInFormatPair& pair = BuiltInFormats[formatID];
                PLY_ASSERT((u32) pair.key == formatID);
                PLY_ASSERT(pair.formatDesc);
                return pair.formatDesc;
            } else {
                return m_schema->userFormatDescs[formatID - FormatID_StartUserRange];
            }
        }

        if (formatKey < (u8) FormatKey::StartUserKeyRange) {
            PLY_ASSERT(!topLevel);
            BuiltInFormatPair& pair = BuiltInFormats[formatKey];
            PLY_ASSERT((u8) pair.key == formatKey);
            return pair.formatDesc;
        }

        FormatDescriptor* newFormat = nullptr;
        u32 userFormatIndex =
            m_schema->userFormatDescs
                .numItems(); // actual formatID is FormatID_StartUserRange + userFormatIndex
        m_schema->userFormatDescs.append(nullptr);
        if (formatKey == (u8) FormatKey::FixedArray) {
            u32 numItems = m_in.read<u32>();
            FormatDescriptor* itemFormat = readFormatDescriptor();
            newFormat = new FormatDescriptor_FixedArray{numItems, itemFormat};
        } else if (formatKey == (u8) FormatKey::Array) {
            FormatDescriptor* itemFormat = readFormatDescriptor();
            newFormat = new FormatDescriptor_Array{itemFormat};
        } else if (formatKey == (u8) FormatKey::Owned) {
            FormatDescriptor* childFormat = readFormatDescriptor();
            newFormat = new FormatDescriptor_Owned{childFormat};
        } else if (formatKey == (u8) FormatKey::WeakPtr) {
            FormatDescriptor* childFormat = readFormatDescriptor();
            newFormat = new FormatDescriptor_WeakPtr{childFormat};
        } else if (formatKey == (u8) FormatKey::Struct) {
            FormatDescriptor_Struct* structFormat = new FormatDescriptor_Struct;
            structFormat->name = Boxed<String>::read(m_in);
            u16 numTemplateParams = m_in.read<u16>();
            u16 numMembers = m_in.read<u16>();
            structFormat->templateParams.resize(numTemplateParams);
            for (FormatDescriptor_Struct::Member& templateParam : structFormat->templateParams) {
                templateParam.name = Boxed<String>::read(m_in);
                templateParam.formatDesc = readFormatDescriptor();
            }
            structFormat->members.resize(numMembers);
            for (FormatDescriptor_Struct::Member& member : structFormat->members) {
                member.name = Boxed<String>::read(m_in);
                member.formatDesc = readFormatDescriptor();
            }
            newFormat = structFormat;
        } else if (formatKey == (u8) FormatKey::Enum) {
            FormatDescriptor_Enum* enumFormat = new FormatDescriptor_Enum;
            enumFormat->name = Boxed<String>::read(m_in);
            enumFormat->fixedSize = m_in.read<u8>();
            u32 numEntries = m_in.read<u32>();
            enumFormat->identifiers.resize(numEntries);
            for (String& name : enumFormat->identifiers) {
                name = Boxed<String>::read(m_in);
            }
            newFormat = enumFormat;
        } else if (formatKey == (u8) FormatKey::EnumIndexedArray) {
            FormatDescriptor_EnumIndexedArray* arrFormat = new FormatDescriptor_EnumIndexedArray;
            arrFormat->itemFormat = readFormatDescriptor();
            FormatDescriptor* enumFormat = readFormatDescriptor();
            PLY_ASSERT(enumFormat->formatKey == (u8) FormatKey::Enum);
            arrFormat->enumFormat = static_cast<FormatDescriptor_Enum*>(enumFormat);
            newFormat = arrFormat;
        } else if (formatKey == (u8) FormatKey::Switch) {
            FormatDescriptor_Switch* switchFormat = new FormatDescriptor_Switch;
            switchFormat->name = Boxed<String>::read(m_in);
            u16 numStates = m_in.read<u16>();
            switchFormat->states.resize(numStates);
            for (FormatDescriptor_Switch::State& state : switchFormat->states) {
                state.name = Boxed<String>::read(m_in);
                FormatDescriptor* stateFormat = readFormatDescriptor();
                PLY_ASSERT(stateFormat->formatKey == (u8) FormatKey::Struct);
                state.structFormat = static_cast<FormatDescriptor_Struct*>(stateFormat);
            }
            newFormat = switchFormat;
        } else {
            PLY_ASSERT(0);
        }
        m_schema->userFormatDescs[userFormatIndex] = newFormat;
        return newFormat;
    }

    void readSchema() {
        // Read all the user FormatDescriptors
        for (;;) {
            if (!readFormatDescriptor(true))
                break;
        }
    }
};

void readSchema(Schema& schema, InStream* in) {
    SchemaLoader loader{&schema, in};
    loader.readSchema();
}

FormatDescriptor* Schema::getFormatDesc(u32 formatID) const {
    if (formatID < FormatID_StartUserRange) {
        // To avoid this assert, and make the serialization system forward-compatible and easy
        // to merge across different branches, perhaps we could add a plugin system that allows
        // arbitrary strings to serve as format keys, instead of limiting it to integers.
        // Format keys are not added very often, but still worth thinking about.
        PLY_ASSERT(formatID <
                   (u32) FormatKey::StartUserKeyRange); // Unrecognized FormatKey in the data
        return BuiltInFormats[formatID].formatDesc;
    } else {
        return userFormatDescs[formatID - FormatID_StartUserRange];
    }
}

} // namespace ply

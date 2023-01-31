/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-reflect/Core.h>

namespace ply {

// The lowest range is reserved for built-in FormatDescriptors
static const u32 FormatID_StartUserRange = 1000;

enum class FormatKey {
    // All FormatKeys less than StartUserKeyRange are also FormatIDs.
    None = 0, // Special value used during serialization
    Indirect, // Special value used during serialization
    Bool,
    S8,
    S16,
    S32,
    S64,
    U8,
    U16,
    U32,
    U64,
    Float,
    Double,
    String,
    TypedArray, // FIXME: Maybe this can be wrapped inside "Typed"
    Typed,
    // Note: If new built-in FormatKeys are added here, it changes the FormatKey of all user
    // FormatDescriptors, so pretty much all data needs to be recooked. Maybe we should allow adding
    // built-in FormatKeys *after* Struct. If so, should remove StartUserKeyRange as it would become
    // meaningless. User FormatDescriptors (not built-in ones) must use formatKey >=
    // StartUserKeyRange
    StartUserKeyRange,
    FixedArray = StartUserKeyRange,
    Array,
    Owned,
    RawPtr,
    Struct,
    Enum,
    Switch
};

struct TypeDescriptor;
struct BuiltInPair {
    FormatKey key;
    TypeDescriptor* type;
};

extern Array<BuiltInPair> BuiltInTypeDescs;

struct FormatDescriptor {
    u8 formatKey;

    FormatDescriptor(u32 formatKey) : formatKey(formatKey) {
    }
    virtual ~FormatDescriptor() {
    } // just for FormatDescriptor_Struct
};

extern FormatDescriptor FormatDescriptor_Bool;
extern FormatDescriptor FormatDescriptor_S8;
extern FormatDescriptor FormatDescriptor_S16;
extern FormatDescriptor FormatDescriptor_S32;
extern FormatDescriptor FormatDescriptor_S64;
extern FormatDescriptor FormatDescriptor_U8;
extern FormatDescriptor FormatDescriptor_U16;
extern FormatDescriptor FormatDescriptor_U32;
extern FormatDescriptor FormatDescriptor_U64;
extern FormatDescriptor FormatDescriptor_Float;
extern FormatDescriptor FormatDescriptor_Double;
extern FormatDescriptor FormatDescriptor_String;
extern FormatDescriptor FormatDescriptor_TypedArray;
extern FormatDescriptor FormatDescriptor_Typed;

struct FormatDescriptor_FixedArray : FormatDescriptor {
    u32 numItems;
    FormatDescriptor* itemFormat;

    FormatDescriptor_FixedArray(u32 numItems, FormatDescriptor* itemFormat)
        : FormatDescriptor((u32) FormatKey::FixedArray), numItems(numItems),
          itemFormat(itemFormat) {
    }
    virtual ~FormatDescriptor_FixedArray() override {
    }
};

struct FormatDescriptor_Array : FormatDescriptor {
    FormatDescriptor* itemFormat;

    FormatDescriptor_Array(FormatDescriptor* itemFormat)
        : FormatDescriptor((u32) FormatKey::Array), itemFormat(itemFormat) {
    }
    virtual ~FormatDescriptor_Array() override {
    }
};

struct FormatDescriptor_Owned : FormatDescriptor {
    FormatDescriptor* childFormat;

    FormatDescriptor_Owned(FormatDescriptor* childFormat)
        : FormatDescriptor((u32) FormatKey::Owned), childFormat(childFormat) {
    }
    virtual ~FormatDescriptor_Owned() override {
    }
};

struct FormatDescriptor_RawPtr : FormatDescriptor {
    FormatDescriptor* childFormat;

    FormatDescriptor_RawPtr(FormatDescriptor* childFormat)
        : FormatDescriptor((u32) FormatKey::RawPtr), childFormat(childFormat) {
    }
    virtual ~FormatDescriptor_RawPtr() override {
    }
};

struct FormatDescriptor_Struct : FormatDescriptor {
    struct Member {
        String name;
        FormatDescriptor* formatDesc;
    };
    String name;
    Array<Member> templateParams;
    Array<Member> members;

    FormatDescriptor* getTemplateParam(StringView name) {
        for (Member& templateParam : templateParams) {
            if (templateParam.name == name)
                return templateParam.formatDesc;
        }
        return nullptr;
    }
    FormatDescriptor* getMember(StringView name) {
        for (Member& member : members) {
            if (member.name == name)
                return member.formatDesc;
        }
        return nullptr;
    }

    FormatDescriptor_Struct() : FormatDescriptor((u32) FormatKey::Struct) {
    }
    virtual ~FormatDescriptor_Struct() override {
    }
};

struct FormatDescriptor_Enum : FormatDescriptor {
    u8 fixedSize; // FIXME: Delete this?
    String name;  // FIXME: Delete this?
    Array<String> identifiers;

    FormatDescriptor_Enum() : FormatDescriptor((u32) FormatKey::Enum) {
    }
    virtual ~FormatDescriptor_Enum() override {
    }
};

struct FormatDescriptor_Switch : FormatDescriptor {
    struct State {
        String name;
        FormatDescriptor_Struct* structFormat;
    };
    String name;
    Array<State> states;

    FormatDescriptor_Switch() : FormatDescriptor((u32) FormatKey::Switch) {
    }
    virtual ~FormatDescriptor_Switch() override {
    }
};

} // namespace ply

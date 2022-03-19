/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-reflect/Core.h>
#include <ply-reflect/TypeDescriptor_Def.h>

namespace ply {

PLY_DLL_ENTRY extern TypeKey TypeKey_Enum;

PLY_DLL_ENTRY NativeBindings& getNativeBindings_Enum();

struct TypeDescriptor_Enum : TypeDescriptor {
    PLY_DLL_ENTRY static TypeKey* typeKey;
    struct Identifier {
        String name;
        u32 value;
    };

    String name;
    Array<Identifier> identifiers;

    PLY_INLINE TypeDescriptor_Enum(u32 fixedSize, const char* name,
                                   std::initializer_list<Identifier> identifiers)
        : TypeDescriptor{&TypeKey_Enum, fixedSize,
                         getNativeBindings_Enum() PLY_METHOD_TABLES_ONLY(, {})},
          name{name}, identifiers{identifiers} {
    }

    PLY_NO_INLINE const Identifier* findIdentifier(StringView name) const {
        for (const Identifier& identifier : identifiers) {
            if (identifier.name == name)
                return &identifier;
        }
        return nullptr;
    }

    PLY_NO_INLINE const Identifier* findValue(u32 value) const {
        for (const Identifier& identifier : identifiers) {
            if (identifier.value == value)
                return &identifier;
        }
        return nullptr;
    }
};

// clang-format off
#define PLY_ENUM_BEGIN(ns, type) \
    PLY_DEFINE_TYPE_DESCRIPTOR(ns type) { \
        using T = ns type; \
        static ply::TypeDescriptor_Enum typeDesc{ \
            sizeof(T), #type, {

#define PLY_ENUM_IDENTIFIER(name) \
                {#name, ply::u32(T::name)},

#define PLY_ENUM_END() \
            } \
        }; \
        return &typeDesc; \
    }
// clang-format on

} // namespace ply

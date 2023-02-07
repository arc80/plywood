/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once
#include <ply-reflect/Core.h>
#include <ply-reflect/TypeDescriptor_Def.h>

namespace ply {

extern TypeKey TypeKey_Enum;

NativeBindings& get_native_bindings_enum();

struct TypeDescriptor_Enum : TypeDescriptor {
    static TypeKey* type_key;
    struct Identifier {
        String name;
        u32 value;
    };

    String name;
    Array<Identifier> identifiers;

    template <typename T>
    TypeDescriptor_Enum(T* typed_arg, const char* name,
                        std::initializer_list<Identifier> identifiers)
        : TypeDescriptor{&TypeKey_Enum, typed_arg,
                         get_native_bindings_enum() PLY_METHOD_TABLES_ONLY(, {})},
          name{name}, identifiers{identifiers} {
    }

    const Identifier* find_identifier(StringView name) const {
        for (const Identifier& identifier : identifiers) {
            if (identifier.name == name)
                return &identifier;
        }
        return nullptr;
    }

    const Identifier* find_value(u32 value) const {
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
        static ply::TypeDescriptor_Enum type_desc{ \
            (T*) nullptr, #type, {

#define PLY_ENUM_IDENTIFIER(name) \
                {#name, ply::u32(T::name)},

#define PLY_ENUM_END() \
            } \
        }; \
        return &type_desc; \
    }
// clang-format on

} // namespace ply

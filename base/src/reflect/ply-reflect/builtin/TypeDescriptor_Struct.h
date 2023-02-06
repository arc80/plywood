/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once
#include <ply-reflect/Core.h>
#include <ply-reflect/TypeDescriptor_Def.h>

#if PLY_WITH_METHOD_TABLES
#include <ply-reflect/methods/BaseInterpreter.h>
#endif // PLY_WITH_METHOD_TABLES

namespace ply {

PLY_DLL_ENTRY extern TypeKey TypeKey_Struct;

PLY_DLL_ENTRY NativeBindings& get_native_bindings_synthesized_struct();
PLY_DLL_ENTRY MethodTable get_method_table_struct();

struct TypeDescriptor_Struct : TypeDescriptor {
    PLY_DLL_ENTRY static TypeKey* type_key;
    struct TemplateParam {
        String name;
        TypeDescriptor* type;
    };
    struct Member {
        String name;
        u32 offset;
        TypeDescriptor* type;
    };

    String name;
    Array<TemplateParam> template_params;
    Array<Member> members;

    // Use expression SFINAE to invoke the object's on_post_serialize() function, if
    // present:
    template <class T>
    static PLY_INLINE auto invoke_post_serialize(T* obj)
        -> decltype(obj->on_post_serialize()) {
        return obj->on_post_serialize();
    }
    static PLY_INLINE void invoke_post_serialize(...) {
    }
    void (*on_post_serialize)(void* data) = [](void*) {};

    // Constructor for synthesized TypeDescriptor_Struct:
    PLY_INLINE TypeDescriptor_Struct(u32 fixed_size, u32 alignment, StringView name)
        : TypeDescriptor{&TypeKey_Struct, fixed_size, alignment,
                         get_native_bindings_synthesized_struct()
                             PLY_METHOD_TABLES_ONLY(, get_method_table_struct())},
          name{name} {
    }
    // Constructor for TypeDescriptor_Struct of an existing C++ class:
    template <class T>
    PLY_INLINE TypeDescriptor_Struct(T* typed_arg, StringView name,
                                     std::initializer_list<Member> members = {})
        : TypeDescriptor{&TypeKey_Struct, typed_arg,
                         NativeBindings::make<T>()
                             PLY_METHOD_TABLES_ONLY(, get_method_table_struct())},
          name{name}, members{members} {
        on_post_serialize = [](void* data) { invoke_post_serialize((T*) data); };
    }

    PLY_INLINE void append_member(StringView name, TypeDescriptor* type) {
        // FIXME: Handle alignment
        members.append({name, fixed_size, type});
        fixed_size += type->fixed_size;
    }

    PLY_NO_INLINE const Member* find_member(StringView name) const {
        for (const Member& member : members) {
            if (member.name == name)
                return &member;
        }
        return nullptr;
    }
};

struct Initializer {
    Initializer(void (*init)()) {
        init();
    }
};

// clang-format off
#define PLY_REFLECT(...) \
    template <typename> friend struct ply::TypeDescriptorSpecializer; \
    static __VA_ARGS__ ply::TypeDescriptor* bind_type_descriptor(); \
    static ply::Initializer init_type_descriptor; \
    static void init_type_descriptor_members();

#define PLY_STRUCT_BEGIN(type) \
    PLY_NO_INLINE ply::TypeDescriptor* type::bind_type_descriptor() { \
        static ply::TypeDescriptor_Struct type_desc{(type*) nullptr, #type}; \
        return &type_desc; \
    } \
    ply::Initializer type::init_type_descriptor{type::init_type_descriptor_members}; \
    void type::init_type_descriptor_members() { \
        using T = type; \
        PLY_UNUSED((T*) nullptr); \
        T::bind_type_descriptor()->cast<ply::TypeDescriptor_Struct>()->members = {

#define PLY_STRUCT_MEMBER(name) \
            {#name, PLY_MEMBER_OFFSET(T, name), ply::get_type_descriptor<decltype(T::name)>()},

#define PLY_STRUCT_END() \
        }; \
    }

#define PLY_STRUCT_BEGIN_PRIM(type) \
    PLY_DEFINE_TYPE_DESCRIPTOR(type) { \
        static ply::TypeDescriptor_Struct type_desc{(type*) nullptr, #type}; \
        return &type_desc; \
    } \
    static void PLY_UNIQUE_VARIABLE(init_type_descriptor_)(); \
    ply::Initializer PLY_UNIQUE_VARIABLE(init_type_descriptor_caller_){PLY_UNIQUE_VARIABLE(init_type_descriptor_)}; \
    static void PLY_UNIQUE_VARIABLE(init_type_descriptor_)() { \
        using T = type; \
        PLY_UNUSED((T*) nullptr); \
        ply::get_type_descriptor<T>()->cast<ply::TypeDescriptor_Struct>()->members = {

#define PLY_STRUCT_END_PRIM() \
        }; \
    }
// clang-format on

} // namespace ply

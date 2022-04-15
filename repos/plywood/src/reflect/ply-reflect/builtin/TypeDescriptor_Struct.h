/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-reflect/Core.h>
#include <ply-reflect/TypeDescriptor_Def.h>

namespace ply {

PLY_DLL_ENTRY extern TypeKey TypeKey_Struct;

PLY_DLL_ENTRY NativeBindings& getNativeBindings_SynthesizedStruct();
PLY_DLL_ENTRY MethodTable getMethodTable_Struct();

struct TypeDescriptor_Struct : TypeDescriptor {
    PLY_DLL_ENTRY static TypeKey* typeKey;
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
    Array<TemplateParam> templateParams;
    Array<Member> members;

    // Use expression SFINAE to invoke the object's onPostSerialize() function, if present:
    template <class T>
    static PLY_INLINE auto invokePostSerialize(T* obj) -> decltype(obj->onPostSerialize()) {
        return obj->onPostSerialize();
    }
    static PLY_INLINE void invokePostSerialize(...) {
    }
    void (*onPostSerialize)(void* data) = [](void*) {};

    // Constructor for synthesized TypeDescriptor_Struct:
    PLY_INLINE TypeDescriptor_Struct(u32 fixedSize, u32 alignment, StringView name)
        : TypeDescriptor{&TypeKey_Struct, fixedSize, alignment,
                         getNativeBindings_SynthesizedStruct() PLY_METHOD_TABLES_ONLY(, getMethodTable_Struct())},
          name{name} {
    }
    // Constructor for TypeDescriptor_Struct of an existing C++ class:
    template <class T>
    PLY_INLINE TypeDescriptor_Struct(T* typedArg, StringView name,
                                     std::initializer_list<Member> members = {})
        : TypeDescriptor{&TypeKey_Struct, typedArg,
                         NativeBindings::make<T>() PLY_METHOD_TABLES_ONLY(, getMethodTable_Struct())},
          name{name}, members{members} {
        onPostSerialize = [](void* data) { invokePostSerialize((T*) data); };
    }

    PLY_INLINE void appendMember(StringView name, TypeDescriptor* type) {
        // FIXME: Handle alignment
        members.append({name, fixedSize, type});
        fixedSize += type->fixedSize;
    }

    PLY_NO_INLINE const Member* findMember(StringView name) const {
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
    static __VA_ARGS__ ply::TypeDescriptor* bindTypeDescriptor(); \
    static ply::Initializer initTypeDescriptor; \
    static void initTypeDescriptorMembers();

#define PLY_STRUCT_BEGIN(type) \
    PLY_NO_INLINE ply::TypeDescriptor* type::bindTypeDescriptor() { \
        static ply::TypeDescriptor_Struct typeDesc{(type*) nullptr, #type}; \
        return &typeDesc; \
    } \
    ply::Initializer type::initTypeDescriptor{type::initTypeDescriptorMembers}; \
    void type::initTypeDescriptorMembers() { \
        using T = type; \
        PLY_UNUSED((T*) nullptr); \
        T::bindTypeDescriptor()->cast<ply::TypeDescriptor_Struct>()->members = {

#define PLY_STRUCT_MEMBER(name) \
            {#name, PLY_MEMBER_OFFSET(T, name), ply::getTypeDescriptor<decltype(T::name)>()},

#define PLY_STRUCT_END() \
        }; \
    }

#define PLY_STRUCT_BEGIN_PRIM(type) \
    PLY_DEFINE_TYPE_DESCRIPTOR(type) { \
        static ply::TypeDescriptor_Struct typeDesc{(type*) nullptr, #type}; \
        return &typeDesc; \
    } \
    static void PLY_UNIQUE_VARIABLE(initTypeDescriptor_)(); \
    ply::Initializer PLY_UNIQUE_VARIABLE(initTypeDescriptorCaller_){PLY_UNIQUE_VARIABLE(initTypeDescriptor_)}; \
    static void PLY_UNIQUE_VARIABLE(initTypeDescriptor_)() { \
        using T = type; \
        PLY_UNUSED((T*) nullptr); \
        ply::getTypeDescriptor<T>()->cast<ply::TypeDescriptor_Struct>()->members = {

#define PLY_STRUCT_END_PRIM() \
        }; \
    }
// clang-format on

} // namespace ply

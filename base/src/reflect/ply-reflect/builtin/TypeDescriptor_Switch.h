/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once
#include <ply-reflect/Core.h>
#include <ply-reflect/builtin/TypeDescriptor_Struct.h>
#include <ply-reflect/builtin/TypeDescriptor_Switch.h>
#include <ply-reflect/AnyObject.h>

namespace ply {

PLY_DLL_ENTRY extern TypeKey TypeKey_Switch;

struct TypeDescriptor_Switch : TypeDescriptor {
    PLY_DLL_ENTRY static TypeKey* type_key;
    struct State {
        String name;
#if PLY_WITH_ASSERTS
        u16 id; // Used only by PLY_SWITCH_END() to ensure that the order of
                // PLY_SWITCH_MEMBER() macros matches the real IDs used at runtime
#endif
        TypeDescriptor_Struct* struct_type;
    };

    String name;
    u16 storage_offset;
    Array<State> states;

    // Constructor for TypeDescriptor_Switch of an existing C++ class:
    template <class T>
    TypeDescriptor_Switch(T* typed_arg, StringView name,
                          std::initializer_list<State> states = {})
        : TypeDescriptor{&TypeKey_Switch, typed_arg,
                         NativeBindings::make<T>() PLY_METHOD_TABLES_ONLY(, {})},
          name{name}, storage_offset{PLY_MEMBER_OFFSET(T, storage)}, states{states} {
    }
    void ensure_state_is(AnyObject obj, u16 state_id) {
        PLY_ASSERT(obj.type == this);
        u16 old_state_id = *(u16*) obj.data;
        if (old_state_id != state_id) {
            void* storage = PLY_PTR_OFFSET(obj.data, storage_offset);
            AnyObject{storage, states[old_state_id].struct_type}.destruct();
            *(u16*) obj.data = state_id;
            AnyObject{storage, states[state_id].struct_type}.construct();
        }
    }
};

// clang-format off
#define PLY_SWITCH_REFLECT(...) \
    template <typename> friend struct ply::TypeDescriptorSpecializer; \
    static __VA_ARGS__ ply::TypeDescriptor* bind_type_descriptor(); \
    static ply::Initializer init_type_descriptor; \
    static void init_type_descriptor_states();

#define PLY_SWITCH_BEGIN(type) \
    PLY_NO_INLINE ply::TypeDescriptor* type::bind_type_descriptor() { \
        static ply::TypeDescriptor_Switch type_desc{(type*) nullptr, #type}; \
        return &type_desc; \
    } \
    ply::Initializer type::init_type_descriptor{type::init_type_descriptor_states}; \
    void type::init_type_descriptor_states() { \
        using T = type; \
        PLY_UNUSED((T*) nullptr); \
        auto* switch_type = T::bind_type_descriptor()->cast<ply::TypeDescriptor_Switch>(); \
        switch_type->states = {

#if PLY_WITH_ASSERTS
#define PLY_SWITCH_MEMBER(id) \
            {#id, (u16) T::ID::id, ply::get_type_descriptor<T::id>()->cast<ply::TypeDescriptor_Struct>()},
#else
#define PLY_SWITCH_MEMBER(id) \
            {#id, ply::get_type_descriptor<T::id>()->cast<ply::TypeDescriptor_Struct>()},
#endif

#define PLY_SWITCH_END() \
        }; \
        PLY_ASSERT(switch_type->states.num_items() == (u32) T::ID::Count); \
        for (u32 i = 0; i < switch_type->states.num_items(); i++) { \
            PLY_ASSERT(switch_type->states[i].id == i); \
        } \
    }
// clang-format on

} // namespace ply

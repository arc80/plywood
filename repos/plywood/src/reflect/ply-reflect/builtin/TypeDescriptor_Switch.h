/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-reflect/Core.h>
#include <ply-reflect/builtin/TypeDescriptor_Struct.h>
#include <ply-reflect/builtin/TypeDescriptor_Switch.h>
#include <ply-reflect/AnyObject.h>

namespace ply {

PLY_DLL_ENTRY extern TypeKey TypeKey_Switch;

struct TypeDescriptor_Switch : TypeDescriptor {
    PLY_DLL_ENTRY static TypeKey* typeKey;
    struct State {
        String name;
#if PLY_WITH_ASSERTS
        u16 id; // Used only by PLY_SWITCH_END() to ensure that the order of
                // PLY_SWITCH_MEMBER() macros matches the real IDs used at runtime
#endif
        TypeDescriptor_Struct* structType;
    };

    String name;
    u16 storageOffset;
    Array<State> states;

    // Constructor for TypeDescriptor_Switch of an existing C++ class:
    template <class T>
    TypeDescriptor_Switch(T*, StringView name, std::initializer_list<State> states = {})
        : TypeDescriptor{&TypeKey_Switch, sizeof(T), NativeBindings::make<T>()}, name{name},
          storageOffset{PLY_MEMBER_OFFSET(T, storage)}, states{states} {
    }
    void ensureStateIs(AnyObject obj, u16 stateID) {
        PLY_ASSERT(obj.type == this);
        u16 oldStateID = *(u16*) obj.data;
        if (oldStateID != stateID) {
            void* storage = PLY_PTR_OFFSET(obj.data, storageOffset);
            AnyObject{storage, states[oldStateID].structType}.destruct();
            *(u16*) obj.data = stateID;
            AnyObject{storage, states[stateID].structType}.construct();
        }
    }
};

// clang-format off
#define PLY_SWITCH_REFLECT(...) \
    template <typename> friend struct ply::TypeDescriptorSpecializer; \
    static __VA_ARGS__ ply::TypeDescriptor* bindTypeDescriptor(); \
    static ply::Initializer initTypeDescriptor; \
    static void initTypeDescriptorStates();

#define PLY_SWITCH_BEGIN(type) \
    PLY_NO_INLINE ply::TypeDescriptor* type::bindTypeDescriptor() { \
        static ply::TypeDescriptor_Switch typeDesc{(type*) nullptr, #type}; \
        return &typeDesc; \
    } \
    ply::Initializer type::initTypeDescriptor{type::initTypeDescriptorStates}; \
    void type::initTypeDescriptorStates() { \
        using T = type; \
        PLY_UNUSED((T*) nullptr); \
        auto* switchType = T::bindTypeDescriptor()->cast<ply::TypeDescriptor_Switch>(); \
        switchType->states = {

#if PLY_WITH_ASSERTS
#define PLY_SWITCH_MEMBER(id) \
            {#id, (u16) T::ID::id, ply::getTypeDescriptor<T::id>()->cast<ply::TypeDescriptor_Struct>()},
#else
#define PLY_SWITCH_MEMBER(id) \
            {#id, ply::getTypeDescriptor<T::id>()},
#endif

#define PLY_SWITCH_END() \
        }; \
        PLY_ASSERT(switchType->states.numItems() == (u32) T::ID::Count); \
        for (u32 i = 0; i < switchType->states.numItems(); i++) { \
            PLY_ASSERT(switchType->states[i].id == i); \
        } \
    }
// clang-format on

} // namespace ply

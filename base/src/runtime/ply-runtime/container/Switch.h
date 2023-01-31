/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime.h>

namespace ply {

struct SwitchType {
    void (*construct)(void*);
    void (*destruct)(void*);
    void (*move)(void*, void*);
    void (*copy)(void*, const void*);

    template <typename T>
    static SwitchType get() {
        return {
            // construct
            [](void* ptr) { new (ptr) T{}; },
            // destruct
            [](void* ptr) { subst::destructByMember((T*) ptr); },
            // move
            [](void* dst, void* src) { subst::unsafeMove<T>((T*) dst, (T*) src); },
            // copy
            [](void* dst, const void* src) { subst::unsafeCopy<T>((T*) dst, (const T*) src); },
        };
    };
};

template <typename Container, typename State, typename Container::ID RequiredID>
struct SwitchWrapper {
    Container& ctr;

    PLY_INLINE SwitchWrapper(Container& ctr) : ctr(ctr) {
    }
    PLY_INLINE explicit operator bool() const {
        return ctr.id == RequiredID;
    }
    template <typename... Args>
    PLY_INLINE SwitchWrapper& switchTo(Args&&... args) {
        Container::idToType[ureg(ctr.id)].destruct(&ctr.storage);
        ctr.id = RequiredID;
        new (&ctr.storage) State{std::forward<Args>(args)...};
        return *this;
    }
    PLY_INLINE const State* operator->() const {
        PLY_ASSERT(ctr.id == RequiredID);
        return (State*) &ctr.storage;
    }
    PLY_INLINE State* operator->() {
        PLY_ASSERT(ctr.id == RequiredID);
        return (State*) &ctr.storage;
    }
    PLY_INLINE const State* get() const {
        PLY_ASSERT(ctr.id == RequiredID);
        return (State*) &ctr.storage;
    }
    PLY_INLINE State* get() {
        PLY_ASSERT(ctr.id == RequiredID);
        return (State*) &ctr.storage;
    }
    PLY_INLINE void operator=(const State& state) {
        PLY_ASSERT(ctr.id == RequiredID);
        *(State*) &ctr.storage = state;
    }
    PLY_INLINE void operator=(State&& state) {
        PLY_ASSERT(ctr.id == RequiredID);
        *(State*) &ctr.storage = std::move(state);
    }
};

#define PLY_STATE(name)

#define SWITCH_FOOTER(ctrName, defaultState) \
    using T = ctrName; \
    ID id; \
    Storage_ storage; \
    static SwitchType idToType[]; \
    template <typename, typename = void> \
    struct TypeToID; \
    PLY_INLINE ctrName() : id{ID::defaultState} { \
        new (&storage) defaultState{}; \
    } \
    PLY_INLINE ~ctrName() { \
        idToType[ply::ureg(id)].destruct(&storage); \
    } \
    PLY_INLINE ctrName(const ctrName& other) : id{other.id} { \
        idToType[ply::ureg(id)].construct(&storage); \
        idToType[ply::ureg(id)].copy(&storage, &other.storage); \
    } \
    PLY_INLINE ctrName(ctrName&& other) : id{other.id} { \
        idToType[ply::ureg(id)].construct(&storage); \
        idToType[ply::ureg(id)].move(&storage, &other.storage); \
    } \
    template <typename S, typename = ply::void_t<decltype(TypeToID<S>::value)>> \
    PLY_INLINE ctrName(S&& other) : id{TypeToID<S>::value} { \
        idToType[ply::ureg(id)].construct(&storage); \
        idToType[ply::ureg(id)].move(&storage, &other); \
    } \
    PLY_INLINE void switchTo(ID newID) { \
        idToType[ply::ureg(id)].destruct(&storage); \
        id = newID; \
        idToType[ply::ureg(id)].construct(&storage); \
    } \
    PLY_INLINE void operator=(const ctrName& other) { \
        switchTo(other.id); \
        idToType[ply::ureg(id)].copy(&storage, &other.storage); \
    } \
    PLY_INLINE void operator=(ctrName&& other) { \
        switchTo(other.id); \
        idToType[ply::ureg(id)].move(&storage, &other.storage); \
    }

// We use partial specialization on TypeToID<> because of
// http://open-std.org/JTC1/SC22/WG21/docs/cwg_defects.html#727
// https://stackoverflow.com/q/49707184/3043469
#define SWITCH_ACCESSOR(state, func) \
    template <typename dummy> \
    struct TypeToID<state, dummy> { \
        static constexpr ID value = ID::state; \
    }; \
    PLY_INLINE ply::SwitchWrapper<T, state, T::ID::state> func() { \
        return {*this}; \
    } \
    PLY_INLINE const ply::SwitchWrapper<const T, state, T::ID::state> func() const { \
        return {*this}; \
    }

#define SWITCH_TABLE_BEGIN(ctrName) ply::SwitchType ctrName::idToType[] = {

#define SWITCH_TABLE_STATE(ctrName, state) ply::SwitchType::get<ctrName::state>(),

#define SWITCH_TABLE_END(ctrName) \
    } \
    ; \
    PLY_STATIC_ASSERT(PLY_STATIC_ARRAY_SIZE(ctrName::idToType) == (ply::ureg) ctrName::ID::Count); \
    PLY_STATIC_ASSERT((ply::ureg) ctrName::ID::Count > 0);

} // namespace ply

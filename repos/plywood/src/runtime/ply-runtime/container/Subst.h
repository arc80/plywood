/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <string.h>

namespace ply {
namespace subst {

// IsRelocatable hasn't been tested.
template <typename T>
constexpr bool IsRelocatable = true; // This hasn't been tested

//-------------------------------------------------------------
// createDefault
//-------------------------------------------------------------
template <typename T>
std::enable_if_t<std::is_arithmetic<T>::value, T> createDefault() {
    return 0;
}

template <typename T>
std::enable_if_t<!std::is_arithmetic<T>::value, T> createDefault() {
    return {};
}

//-------------------------------------------------------------
// unsafeConstruct
//-------------------------------------------------------------
template <typename T>
static std::enable_if_t<std::is_default_constructible<T>::value, void> unsafeConstruct(T* obj) {
    new (obj) T;
}

template <typename T>
static std::enable_if_t<!std::is_default_constructible<T>::value, void> unsafeConstruct(T* obj) {
    // Not constructible
    PLY_FORCE_CRASH();
}

//-------------------------------------------------------------
// createByMember
//-------------------------------------------------------------
PLY_SFINAE_EXPR_1(HasCreate, T0::create())

template <typename T>
static std::enable_if_t<HasCreate<T>, T*> createByMember() {
    return T::create();
}

template <typename T>
static std::enable_if_t<!HasCreate<T> && std::is_default_constructible<T>::value, T*>
createByMember() {
    return new T;
}

template <typename T>
static std::enable_if_t<!HasCreate<T> && !std::is_default_constructible<T>::value, T*>
createByMember() {
    // Not constructible
    PLY_FORCE_CRASH();
    return nullptr;
}

//-------------------------------------------------------------
// destroyByMember
//-------------------------------------------------------------
PLY_SFINAE_EXPR_1(HasDestroy, std::declval<T0>().destroy())

template <typename T>
static std::enable_if_t<HasDestroy<T>> destroyByMember(T* obj) {
    if (obj) {
        obj->destroy();
    }
}

template <typename T>
static std::enable_if_t<!HasDestroy<T>> destroyByMember(T* obj) {
    delete obj; // Passing nullptr to delete is allowed
}

//-------------------------------------------------------------
// destructByMember
//-------------------------------------------------------------
PLY_SFINAE_EXPR_1(HasDestruct, std::declval<T0>().destruct())

template <typename T>
static std::enable_if_t<HasDestruct<T>> destructByMember(T* obj) {
    obj->destruct();
}

template <typename T>
static std::enable_if_t<!HasDestruct<T>> destructByMember(T* obj) {
    obj->~T();
}

//-------------------------------------------------------------
// unsafeMoveConstruct
//-------------------------------------------------------------
template <typename T>
static std::enable_if_t<std::is_move_constructible<T>::value> unsafeMoveConstruct(T* dst, T* src) {
    new (dst) T{std::move(*src)};
}

template <typename T>
static std::enable_if_t<!std::is_move_constructible<T>::value> unsafeMoveConstruct(T* dst, T* src) {
    // Not move constructible
    PLY_FORCE_CRASH();
}

//-------------------------------------------------------------
// unsafeCopy
//-------------------------------------------------------------
template <typename T>
static std::enable_if_t<std::is_copy_assignable<T>::value> unsafeCopy(T* dst, const T* src) {
    *dst = *src;
}

template <typename T>
static std::enable_if_t<!std::is_copy_assignable<T>::value> unsafeCopy(T* dst, const T* src) {
    // Not copy assignable
    PLY_FORCE_CRASH();
}

//-------------------------------------------------------------
// unsafeMove
//-------------------------------------------------------------
template <typename T>
static std::enable_if_t<std::is_move_assignable<T>::value> unsafeMove(T* dst, T* src) {
    *dst = std::move(*src);
}

template <typename T>
static std::enable_if_t<!std::is_move_assignable<T>::value> unsafeMove(T* dst, T* src) {
    // Not move assignable
    PLY_FORCE_CRASH();
}

//-------------------------------------------------------------
// constructArray
//-------------------------------------------------------------
template <typename T>
static std::enable_if_t<std::is_trivially_default_constructible<T>::value>
constructArray(T* items, s32 size) {
    // Trivially constructible
}

template <typename T>
static std::enable_if_t<!std::is_trivially_default_constructible<T>::value>
constructArray(T* items, s32 size) {
    // Explicitly constructble
    while (size-- > 0) {
        new (items++) T;
    }
}

//-------------------------------------------------------------
// destructArray
//-------------------------------------------------------------
template <typename T>
static std::enable_if_t<std::is_trivially_destructible<T>::value> destructArray(T* items,
                                                                                s32 size) {
    // Trivially destructible
}

template <typename T>
static std::enable_if_t<!std::is_trivially_destructible<T>::value> destructArray(T* items,
                                                                                 s32 size) {
    // Explicitly destructble
    while (size-- > 0) {
        (items++)->~T();
    }
}

//-------------------------------------------------------------
// constructArrayFrom
//-------------------------------------------------------------
template <typename T>
static std::enable_if_t<std::is_trivially_copy_constructible<T>::value>
constructArrayFrom(T* dst, const T* src, s32 size) {
    // Trivially copy constructible
    if (size > 0) {
        memcpy(dst, src, sizeof(T) * size);
    }
}

template <typename T, typename U>
static std::enable_if_t<std::is_constructible<T, const U&>::value>
constructArrayFrom(T* dst, const U* src, s32 size) {
    // Invoke constructor explicitly on each item
    while (size-- > 0) {
        new (dst++) T{*src++};
    }
}

//-------------------------------------------------------------
// unsafeConstructArrayFrom
//-------------------------------------------------------------
template <typename T, typename U>
static std::enable_if_t<std::is_constructible<T, const U&>::value>
unsafeConstructArrayFrom(T* dst, const U* src, s32 size) {
    constructArrayFrom(dst, src, size);
}

template <typename T, typename U>
static std::enable_if_t<!std::is_constructible<T, const U&>::value>
unsafeConstructArrayFrom(T* dst, const U* src, s32 size) {
    PLY_FORCE_CRASH();
}

//-------------------------------------------------------------
// moveConstructArray
//-------------------------------------------------------------
template <typename T>
static std::enable_if_t<std::is_trivially_move_constructible<T>::value>
moveConstructArray(T* dst, const T* src, s32 size) {
    // Trivially move constructible
    if (size > 0) {
        memcpy(dst, src, sizeof(T) * size);
    }
}

template <typename T>
static std::enable_if_t<!std::is_trivially_move_constructible<T>::value>
moveConstructArray(T* dst, T* src, s32 size) {
    // Explicitly move constructible
    while (size-- > 0) {
        new (dst++) T{std::move(*src++)};
    }
}

//-------------------------------------------------------------
// copyArray
//-------------------------------------------------------------
template <typename T>
static std::enable_if_t<std::is_trivially_copy_assignable<T>::value> copyArray(T* dst, const T* src,
                                                                               s32 size) {
    // Trivially copy assignable
    if (size > 0) {
        memcpy(dst, src, sizeof(T) * size);
    }
}

template <typename T>
static std::enable_if_t<!std::is_trivially_copy_assignable<T>::value>
copyArray(T* dst, const T* src, s32 size) {
    // Explicitly copy assignable
    while (size-- > 0) {
        *dst++ = *src++;
    }
}

//-------------------------------------------------------------
// moveArray
//-------------------------------------------------------------
template <typename T>
static std::enable_if_t<std::is_trivially_move_assignable<T>::value> moveArray(T* dst, const T* src,
                                                                               s32 size) {
    // Trivially move assignable
    if (size > 0) {
        memcpy(dst, src, sizeof(T) * size);
    }
}

template <typename T>
static std::enable_if_t<!std::is_trivially_move_assignable<T>::value> moveArray(T* dst, T* src,
                                                                                s32 size) {
    // Explicitly move assignable
    while (size-- > 0) {
        *dst++ = std::move(*src++);
    }
}

} // namespace subst
} // namespace ply

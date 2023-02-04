/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once
#include <ply-reflect/Core.h>
#include <ply-reflect/TypeDescriptor.h>

namespace ply {

//---------------------------------------------------
// StaticPtr
//---------------------------------------------------
struct BaseStaticPtr {
    struct PossibleValues {
        Array<StringView> enumeratorNames;
        Array<void*> ptrValues;
    };
};

template <typename T>
class StaticPtr {
private:
    T* ptr = nullptr;

public:
    PLY_INLINE StaticPtr(T* ptr = nullptr) : ptr{ptr} {
    }
    PLY_INLINE ~StaticPtr() {
    }
    PLY_INLINE void operator=(const StaticPtr& other) {
        this->ptr = other.ptr;
    }
    PLY_INLINE T* operator->() const {
        return this->ptr;
    }
    PLY_INLINE operator T*() const {
        return this->ptr;
    }
    PLY_INLINE T* get() const {
        return this->ptr;
    }
};

//---------------------------------------------------
// TypeDescriptor_StaticPtr
//---------------------------------------------------
PLY_DLL_ENTRY extern TypeKey TypeKey_StaticPtr;
PLY_DLL_ENTRY NativeBindings& getNativeBindings_StaticPtr();

struct TypeDescriptor_StaticPtr : TypeDescriptor {
    static constexpr TypeKey* typeKey = &TypeKey_StaticPtr;

    // Note: for now, we must manually initialize possibleValues at app startup
    BaseStaticPtr::PossibleValues* possibleValues = nullptr;

    TypeDescriptor_StaticPtr()
        : TypeDescriptor{&TypeKey_StaticPtr, (StaticPtr<void>*) nullptr,
                         getNativeBindings_StaticPtr() PLY_METHOD_TABLES_ONLY(, {})} {
    }
};

template <typename T>
struct TypeDescriptorSpecializer<StaticPtr<T>> {
    static TypeDescriptor_StaticPtr* get() {
        static TypeDescriptor_StaticPtr typeDesc;
        return &typeDesc;
    }
};

} // namespace ply

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
        Array<StringView> enumerator_names;
        Array<void*> ptr_values;
    };
};

template <typename T>
class StaticPtr {
private:
    T* ptr = nullptr;

public:
    StaticPtr(T* ptr = nullptr) : ptr{ptr} {
    }
    ~StaticPtr() {
    }
    void operator=(const StaticPtr& other) {
        this->ptr = other.ptr;
    }
    T* operator->() const {
        return this->ptr;
    }
    operator T*() const {
        return this->ptr;
    }
    T* get() const {
        return this->ptr;
    }
};

//---------------------------------------------------
// TypeDescriptor_StaticPtr
//---------------------------------------------------
extern TypeKey TypeKey_StaticPtr;
NativeBindings& get_native_bindings_static_ptr();

struct TypeDescriptor_StaticPtr : TypeDescriptor {
    static constexpr TypeKey* type_key = &TypeKey_StaticPtr;

    // Note: for now, we must manually initialize possible_values at app startup
    BaseStaticPtr::PossibleValues* possible_values = nullptr;

    TypeDescriptor_StaticPtr()
        : TypeDescriptor{&TypeKey_StaticPtr, (StaticPtr<void>*) nullptr,
                         get_native_bindings_static_ptr()
                             PLY_METHOD_TABLES_ONLY(, {})} {
    }
};

template <typename T>
struct TypeDescriptorSpecializer<StaticPtr<T>> {
    static TypeDescriptor_StaticPtr* get() {
        static TypeDescriptor_StaticPtr type_desc;
        return &type_desc;
    }
};

} // namespace ply

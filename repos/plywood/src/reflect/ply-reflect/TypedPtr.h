/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-reflect/Core.h>

namespace ply {

struct TypeDescriptor;

//-----------------------------------------------------------------------
//  TypedPtr
//-----------------------------------------------------------------------
struct TypedPtr {
    void* ptr = nullptr;
    TypeDescriptor* type = nullptr;

    TypedPtr() = default;
    TypedPtr(void* ptr, TypeDescriptor* type) : ptr{ptr}, type{type} {
    }

    template <typename T>
    static TypedPtr bind(T* ptr);
    template <class T>
    bool is() const;
    template <class T>
    T* cast() const;
    template <class T>
    T* safeCast() const;
    template <class S>
    const S& refine() const;

    static TypedPtr create(TypeDescriptor* typeDesc);
    void destroy();
    void construct();
    void destruct();
    void move(TypedPtr other);
    void copy(const TypedPtr other);

    PLY_INLINE bool operator==(const TypedPtr& other) const {
        return this->ptr == other.ptr && this->type == other.type;
    }
};

} // namespace ply

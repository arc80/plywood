/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-reflect/Core.h>
#include <ply-reflect/AnyObject_Def.h>
#include <ply-reflect/TypeDescriptor_Def.h>

namespace ply {

template <typename T>
AnyObject AnyObject::bind(T* data) {
    // FIXME: Find a better way to handle cases where this function is passed a pointer to
    // const.
    return AnyObject{(void*) data, TypeDescriptorSpecializer<T>::get()};
}

template <class T>
bool AnyObject::is() const {
    return (TypeDescriptorSpecializer<T>::get() == type);
}

template <class T>
T* AnyObject::cast() const {
    // Not sure if this is a good general strategy, but for now, it's valid to cast a null
    // pointer to any target type, regardless of src type (even if src type is null). This
    // extra flexibility was added to simplify callers of readObject(), such as
    // CookCommandReader(), when an unexpected EOF is encountered:
    PLY_ASSERT(!data || (TypeDescriptorSpecializer<T>::get() == type));
    return (T*) data;
}

template <class T>
T* AnyObject::safeCast() const {
    return (TypeDescriptorSpecializer<T>::get() == this->type) ? (T*) data : nullptr;
}

template <class S>
const S& AnyObject::refine() const {
    PLY_ASSERT(type->typeKey == S::TypeDescriptor::typeKey);
    return (const S&) *this;
}

inline AnyObject AnyObject::create(TypeDescriptor* typeDesc) {
    return typeDesc->bindings.create(typeDesc);
}

inline void AnyObject::destroy() {
    if (data) {
        type->bindings.destroy(*this);
    }
}

inline void AnyObject::construct() {
    type->bindings.construct(*this);
}

inline void AnyObject::destruct() {
    type->bindings.destruct(*this);
}

inline void AnyObject::move(AnyObject other) {
    type->bindings.move(*this, other);
}

inline void AnyObject::copy(const AnyObject other) {
    type->bindings.copy(*this, other);
}

} // namespace ply

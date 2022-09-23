/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-reflect/Core.h>
#include <ply-reflect/TypeKey.h>
#include <ply-reflect/AnyObject_Def.h>

#if PLY_WITH_METHOD_TABLES
#include <ply-reflect/methods/MethodTable.h>
#endif // PLY_WITH_METHOD_TABLES

namespace ply {

struct TypeDescriptor;

//--------------------------------------------------------
// TypeDescriptors are created for non-aggregate C++ types using the
// PLY_DECLARE/DEFINE_TYPE_DESCRIPTOR() macros. Each time getTypeDescriptor<>() is instantiated for
// a specific type at compile time, a TypeDescriptor corresponding to that type will be made
// available at runtime.
//--------------------------------------------------------
template <typename T>
struct TypeDescriptorSpecializer {
    static PLY_INLINE TypeDescriptor* get() {
        return T::bindTypeDescriptor();
    }
};

template <typename T>
PLY_INLINE TypeDescriptor* getTypeDescriptor(T* = nullptr) {
    return TypeDescriptorSpecializer<T>::get();
}

#define PLY_DECLARE_TYPE_DESCRIPTOR(typeName, ...) \
    template <> \
    struct ply::TypeDescriptorSpecializer<typeName> { \
        static __VA_ARGS__ ply::TypeDescriptor* get(); \
    };

#define PLY_DEFINE_TYPE_DESCRIPTOR(typeName) \
    PLY_NO_INLINE ply::TypeDescriptor* ply::TypeDescriptorSpecializer<typeName>::get()

//-----------------------------------------------------------------------
// NativeBindings
//-----------------------------------------------------------------------
struct NativeBindings {
    AnyObject (*create)(TypeDescriptor* typeDesc);
    void (*destroy)(AnyObject obj);
    void (*construct)(AnyObject obj);
    void (*destruct)(AnyObject obj);
    void (*move)(AnyObject dst, AnyObject src);
    void (*copy)(AnyObject dst, const AnyObject src);

    template <typename T>
    static NativeBindings make() {
        return {
            // create
            [](TypeDescriptor* typeDesc) -> AnyObject {
                PLY_ASSERT(typeDesc == TypeDescriptorSpecializer<T>::get());
                return {subst::createByMember<T>(), typeDesc};
            },
            // destroy
            [](AnyObject obj) {
                PLY_ASSERT(obj.type == TypeDescriptorSpecializer<T>::get());
                subst::destroyByMember((T*) obj.data);
            },
            // construct
            [](AnyObject obj) {
                PLY_ASSERT(obj.type == TypeDescriptorSpecializer<T>::get());
                subst::unsafeConstruct((T*) obj.data);
            },
            // destruct
            [](AnyObject obj) {
                PLY_ASSERT(obj.type == TypeDescriptorSpecializer<T>::get());
                subst::destructByMember((T*) obj.data);
            },
            // move
            [](AnyObject dst, AnyObject src) {
                PLY_ASSERT(dst.type == TypeDescriptorSpecializer<T>::get());
                PLY_ASSERT(src.type == TypeDescriptorSpecializer<T>::get());
                subst::unsafeMove((T*) dst.data, (T*) src.data);
            },
            // copy
            [](AnyObject dst, const AnyObject src) {
                PLY_ASSERT(dst.type == TypeDescriptorSpecializer<T>::get());
                PLY_ASSERT(src.type == TypeDescriptorSpecializer<T>::get());
                subst::unsafeCopy((T*) dst.data, (const T*) src.data);
            },
        };
    }
};

//-----------------------------------------------------------------------
// TypeDescriptor
//-----------------------------------------------------------------------
struct TypeDescriptor {
    TypeKey* typeKey;
    u32 fixedSize;
    u32 alignment;
    NativeBindings bindings;
    PLY_METHOD_TABLES_ONLY(MethodTable methods;)

    template <typename T>
    TypeDescriptor(TypeKey* typeKey, T*,
                   const NativeBindings& bindings
                       PLY_METHOD_TABLES_ONLY(, const MethodTable& methods))
        : typeKey{typeKey}, fixedSize{sizeof(T)}, alignment{alignof(T)},
          bindings{bindings} PLY_METHOD_TABLES_ONLY(, methods{methods}) {
    }
    TypeDescriptor(TypeKey* typeKey, u32 fixedSize, u32 alignment,
                   const NativeBindings& bindings
                       PLY_METHOD_TABLES_ONLY(, const MethodTable& methods))
        : typeKey{typeKey}, fixedSize{fixedSize}, alignment{alignment},
          bindings{bindings} PLY_METHOD_TABLES_ONLY(, methods{methods}) {
    }
    TypeDescriptor(TypeDescriptor&& other)
        : typeKey{other.typeKey}, fixedSize{other.fixedSize}, alignment{other.alignment},
          bindings{other.bindings} PLY_METHOD_TABLES_ONLY(, methods{other.methods}) {
    }
    // Built-in TypeDescriptors exist for the entire process lifetime, including those defined
    // by PLY_STRUCT_BEGIN. The only TypeDescriptors that ever get destroyed are ones that are
    // synthesized at runtime. The lifetime of those TypeDescriptors is managed by a
    // TypeDescriptorOwner, which knows how to correctly destruct them.
    template <class T>
    T* cast() {
        PLY_ASSERT(typeKey == T::typeKey);
        return static_cast<T*>(this);
    }
    template <class T>
    const T* cast() const {
        PLY_ASSERT(typeKey == T::typeKey);
        return static_cast<const T*>(this);
    }

    PLY_INLINE HybridString getName() const {
        return this->typeKey->getName(this);
    }
};

PLY_INLINE Hasher& operator<<(Hasher& hasher, const TypeDescriptor* typeDesc) {
    hasher << typeDesc->typeKey;
    typeDesc->typeKey->hashDescriptor(hasher, typeDesc);
    return hasher;
}

//-----------------------------------------------------------------------
// TypeDescriptorSpecializer for const types
//-----------------------------------------------------------------------
template <typename T>
struct TypeDescriptorSpecializer<const T> {
    static PLY_INLINE auto* get() {
        return TypeDescriptorSpecializer<T>::get();
    }
};

} // namespace ply

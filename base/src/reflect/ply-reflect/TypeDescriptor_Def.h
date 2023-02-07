/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
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
// PLY_DECLARE/DEFINE_TYPE_DESCRIPTOR() macros. Each time get_type_descriptor<>() is
// instantiated for a specific type at compile time, a TypeDescriptor corresponding to
// that type will be made available at runtime.
//--------------------------------------------------------
template <typename T>
struct TypeDescriptorSpecializer {
    static TypeDescriptor* get() {
        return T::bind_type_descriptor();
    }
};

template <typename T>
TypeDescriptor* get_type_descriptor(T* = nullptr) {
    return TypeDescriptorSpecializer<T>::get();
}

#define PLY_DECLARE_TYPE_DESCRIPTOR(type_name, ...) \
    template <> \
    struct ply::TypeDescriptorSpecializer<type_name> { \
        static __VA_ARGS__ ply::TypeDescriptor* get(); \
    };

#define PLY_DEFINE_TYPE_DESCRIPTOR(type_name) \
    ply::TypeDescriptor* ply::TypeDescriptorSpecializer<type_name>::get()

//-----------------------------------------------------------------------
// NativeBindings
//-----------------------------------------------------------------------
struct NativeBindings {
    AnyObject (*create)(TypeDescriptor* type_desc);
    void (*destroy)(AnyObject obj);
    void (*construct)(AnyObject obj);
    void (*destruct)(AnyObject obj);
    void (*move)(AnyObject dst, AnyObject src);
    void (*copy)(AnyObject dst, const AnyObject src);

    template <typename T>
    static NativeBindings make() {
        return {
            // create
            [](TypeDescriptor* type_desc) -> AnyObject {
                PLY_ASSERT(type_desc == TypeDescriptorSpecializer<T>::get());
                return {subst::create_by_member<T>(), type_desc};
            },
            // destroy
            [](AnyObject obj) {
                PLY_ASSERT(obj.type == TypeDescriptorSpecializer<T>::get());
                subst::destroy_by_member((T*) obj.data);
            },
            // construct
            [](AnyObject obj) {
                PLY_ASSERT(obj.type == TypeDescriptorSpecializer<T>::get());
                subst::unsafe_construct((T*) obj.data);
            },
            // destruct
            [](AnyObject obj) {
                PLY_ASSERT(obj.type == TypeDescriptorSpecializer<T>::get());
                subst::destruct_by_member((T*) obj.data);
            },
            // move
            [](AnyObject dst, AnyObject src) {
                PLY_ASSERT(dst.type == TypeDescriptorSpecializer<T>::get());
                PLY_ASSERT(src.type == TypeDescriptorSpecializer<T>::get());
                subst::unsafe_move((T*) dst.data, (T*) src.data);
            },
            // copy
            [](AnyObject dst, const AnyObject src) {
                PLY_ASSERT(dst.type == TypeDescriptorSpecializer<T>::get());
                PLY_ASSERT(src.type == TypeDescriptorSpecializer<T>::get());
                subst::unsafe_copy((T*) dst.data, (const T*) src.data);
            },
        };
    }
};

//-----------------------------------------------------------------------
// TypeDescriptor
//-----------------------------------------------------------------------
struct TypeDescriptor {
    TypeKey* type_key;
    u32 fixed_size;
    u32 alignment;
    NativeBindings bindings;
    PLY_METHOD_TABLES_ONLY(MethodTable methods;)

    template <typename T>
    TypeDescriptor(TypeKey* type_key, T*,
                   const NativeBindings& bindings
                       PLY_METHOD_TABLES_ONLY(, const MethodTable& methods))
        : type_key{type_key}, fixed_size{sizeof(T)}, alignment{alignof(T)},
          bindings{bindings} PLY_METHOD_TABLES_ONLY(, methods{methods}) {
    }
    TypeDescriptor(TypeKey* type_key, u32 fixed_size, u32 alignment,
                   const NativeBindings& bindings
                       PLY_METHOD_TABLES_ONLY(, const MethodTable& methods))
        : type_key{type_key}, fixed_size{fixed_size}, alignment{alignment},
          bindings{bindings} PLY_METHOD_TABLES_ONLY(, methods{methods}) {
    }
    TypeDescriptor(TypeDescriptor&& other)
        : type_key{other.type_key}, fixed_size{other.fixed_size},
          alignment{other.alignment}, bindings{other.bindings} PLY_METHOD_TABLES_ONLY(
                                          , methods{other.methods}) {
    }
    // Built-in TypeDescriptors exist for the entire process lifetime, including those
    // defined by PLY_STRUCT_BEGIN. The only TypeDescriptors that ever get destroyed are
    // ones that are synthesized at runtime. The lifetime of those TypeDescriptors is
    // managed by a TypeDescriptorOwner, which knows how to correctly destruct them.
    template <class T>
    T* cast() {
        PLY_ASSERT(type_key == T::type_key);
        return static_cast<T*>(this);
    }
    template <class T>
    const T* cast() const {
        PLY_ASSERT(type_key == T::type_key);
        return static_cast<const T*>(this);
    }

    HybridString get_name() const {
        return this->type_key->get_name(this);
    }
};

inline Hasher& operator<<(Hasher& hasher, const TypeDescriptor* type_desc) {
    hasher << type_desc->type_key;
    type_desc->type_key->hash_descriptor(hasher, type_desc);
    return hasher;
}

//-----------------------------------------------------------------------
// TypeDescriptorSpecializer for const types
//-----------------------------------------------------------------------
template <typename T>
struct TypeDescriptorSpecializer<const T> {
    static auto* get() {
        return TypeDescriptorSpecializer<T>::get();
    }
};

} // namespace ply

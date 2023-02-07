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

// This class exists to support ply::TypedArray and gpu::ArrayBuffer.
// In general, it can manage the lifetime of any TypeDescriptor, including all its child
// TypeDescriptors. The entire set of TypeDescriptors will be destroyed as a group.
// Future idea: Allocate this group of TypeDescriptors in a single contiguous memory
// block?
class TypeDescriptorOwner : public DualRefCounted<TypeDescriptorOwner> {
protected:
    friend class DualRefCounted<TypeDescriptorOwner>;
    Array<Owned<TypeDescriptor>> m_synthesizedTypes;
    TypeDescriptor* m_rootType =
        nullptr; // This might not actually be one of the
                 // m_synthesizedTypes; eg. index buffers just contain u16

    void on_partial_ref_count_zero() { // Called from DualRefCounted mixin
        m_synthesizedTypes.clear();
        m_rootType = nullptr;
    }
    void on_full_ref_count_zero() { // Called from DualRefCounted mixin
        PLY_ASSERT(
            !m_rootType); // Because on_partial_ref_count_zero() must have been called
        delete this;
    }

public:
    void adopt_type(TypeDescriptor* type) {
        m_synthesizedTypes.append(type);
    }
    void set_root_type(TypeDescriptor* root_type) {
        m_rootType = root_type;
    }
    TypeDescriptor* get_root_type() const {
        return m_rootType;
    }
};

struct EmptyType {
    PLY_REFLECT()
    // ply reflect off
};

template <typename T>
struct TypeOwnerResolver {
    static TypeDescriptorOwner* get() {
        static TypeDescriptorOwner owner;
        if (owner.get_ref_count() == 0) {
            owner.inc_ref(); // Never deleted
            PLY_ASSERT(!owner.get_root_type());
            owner.set_root_type(get_type_descriptor<T>());
        }
        return &owner;
    }
};

struct TypeOwnerPtr {
    void* data = nullptr;
    TypeDescriptorOwner* type_owner = nullptr;

    TypeOwnerPtr() = default;
    TypeOwnerPtr(void* data, TypeDescriptorOwner* type_owner)
        : data{data}, type_owner{type_owner} {
    }

    template <typename T>
    static TypeOwnerPtr bind(T* data) {
        // FIXME: Find a better way to handle cases where this function is passed a
        // pointer to const.
        return {(void*) data, TypeOwnerResolver<T>::get()};
    }
};

} // namespace ply

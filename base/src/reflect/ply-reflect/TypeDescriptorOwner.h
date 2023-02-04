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
// TypeDescriptors. The entire set of TypeDescriptors will be destroyed as a group. Future
// idea: Allocate this group of TypeDescriptors in a single contiguous memory block?
class TypeDescriptorOwner : public DualRefCounted<TypeDescriptorOwner> {
protected:
    friend class DualRefCounted<TypeDescriptorOwner>;
    Array<Owned<TypeDescriptor>> m_synthesizedTypes;
    TypeDescriptor* m_rootType = nullptr; // This might not actually be one of the
                                          // m_synthesizedTypes; eg. index buffers just contain u16

    void onPartialRefCountZero() { // Called from DualRefCounted mixin
        m_synthesizedTypes.clear();
        m_rootType = nullptr;
    }
    void onFullRefCountZero() {  // Called from DualRefCounted mixin
        PLY_ASSERT(!m_rootType); // Because onPartialRefCountZero() must have been called
        delete this;
    }

public:
    void adoptType(TypeDescriptor* type) {
        m_synthesizedTypes.append(type);
    }
    void setRootType(TypeDescriptor* rootType) {
        m_rootType = rootType;
    }
    TypeDescriptor* getRootType() const {
        return m_rootType;
    }
};

struct EmptyType {
    PLY_REFLECT(PLY_DLL_ENTRY)
    // ply reflect off
};

template <typename T>
struct TypeOwnerResolver {
    static TypeDescriptorOwner* get() {
        static TypeDescriptorOwner owner;
        if (owner.getRefCount() == 0) {
            owner.incRef(); // Never deleted
            PLY_ASSERT(!owner.getRootType());
            owner.setRootType(getTypeDescriptor<T>());
        }
        return &owner;
    }
};

struct TypeOwnerPtr {
    void* data = nullptr;
    TypeDescriptorOwner* typeOwner = nullptr;

    TypeOwnerPtr() = default;
    TypeOwnerPtr(void* data, TypeDescriptorOwner* typeOwner) : data{data}, typeOwner{typeOwner} {
    }

    template <typename T>
    static TypeOwnerPtr bind(T* data) {
        // FIXME: Find a better way to handle cases where this function is passed a pointer to
        // const.
        return {(void*) data, TypeOwnerResolver<T>::get()};
    }
};

} // namespace ply

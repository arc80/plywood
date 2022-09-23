/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-reflect/Core.h>
#include <ply-reflect/AnyObject.h>

namespace ply {

struct AnyOwnedObject : AnyObject {
    AnyOwnedObject() {
    }
    AnyOwnedObject(void* data, TypeDescriptor* type) : AnyObject{data, type} {
    }
    AnyOwnedObject(AnyObject&& other) : AnyObject{other.data, other.type} {
    }
    AnyOwnedObject(AnyOwnedObject&& other) : AnyObject{other.data, other.type} {
        other.data = nullptr;
        other.type = nullptr;
    }
    void operator=(AnyObject&& other) {
        destroy();
        data = other.data;
        type = other.type;
    }
    void operator=(AnyOwnedObject&& other) {
        destroy();
        data = other.data;
        type = other.type;
        other.data = nullptr;
        other.type = nullptr;
    }
    ~AnyOwnedObject() {
        destroy();
    }
    template <typename T>
    void assign(Owned<T>&& other) {
        destroy();
        data = other.release();
        type = getTypeDescriptor<T>();
    }
    template <typename T>
    static AnyOwnedObject bind(T* data) {
        // FIXME: Find a better way to handle cases where this function is passed a pointer to
        // const.
        return AnyOwnedObject{(void*) data, TypeDescriptorSpecializer<T>::get()};
    }
    template <typename T, typename... Args>
    static AnyOwnedObject create(Args&&... args) {
        return bind(new T{std::forward<Args>(args)...});
    }
    static AnyOwnedObject create(TypeDescriptor* typeDesc) {
        void* data = PLY_HEAP.alloc(typeDesc->fixedSize);
        AnyOwnedObject result{data, typeDesc};
        result.construct();
        return result;
    }
    template <typename T>
    PLY_INLINE T* release() {
        PLY_ASSERT(type == TypeDescriptorSpecializer<T>::get());
        T* r = (T*) data;
        data = nullptr;
        type = nullptr;
        return r;
    }
};

template <>
struct TypeDescriptorSpecializer<AnyOwnedObject> {
    static PLY_DLL_ENTRY ply::TypeDescriptor* get();
};

} // namespace ply

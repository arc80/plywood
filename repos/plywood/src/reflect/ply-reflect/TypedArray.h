/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-reflect/Core.h>
#include <ply-reflect/TypeDescriptorOwner.h>

namespace ply {

struct TypedArrayView {
    void* items;
    u32 numItems;
    TypeDescriptorOwner* itemTypeOwner;

    PLY_INLINE TypedArrayView(void* items, u32 numItems, TypeDescriptorOwner* itemTypeOwner)
        : items{items}, numItems{numItems}, itemTypeOwner{itemTypeOwner} {
    }

    PLY_INLINE u32 sizeBytes() const {
        return numItems * itemTypeOwner->getRootType()->fixedSize;
    }

    template <typename T>
    static PLY_INLINE TypedArrayView from(const ArrayView<T>& view) {
        // FIXME: Decide whether we want actually keep a const void* if T is const! (See
        // NormalParticleSmoke::upload for example)
        return {(void*) view.items, view.numItems, TypeOwnerResolver<T>::get()};
    }

    static PLY_DLL_ENTRY void constructSlow(TypedArrayView view);
    static PLY_DLL_ENTRY void destructSlow(TypedArrayView view);

    PLY_INLINE void construct() {
        TypeDescriptor* itemType = itemTypeOwner->getRootType();
        if (itemType->bindings.construct != nullptr) { // FIXME: Detect trivial constructors
            constructSlow(*this);
        }
    }

    PLY_INLINE void destruct() {
        TypeDescriptor* itemType = itemTypeOwner->getRootType();
        if (itemType->bindings.destruct != nullptr) { // FIXME: Detect trivial destructors
            destructSlow(*this);
        }
    }
};

class TypedArray {
public:
    details::BaseArray m_array;
    Reference<TypeDescriptorOwner> m_typeOwner = TypeOwnerResolver<EmptyType>::get();

    PLY_INLINE TypedArrayView view() {
        return {m_array.m_items, m_array.m_numItems, m_typeOwner};
    }

    PLY_DLL_ENTRY void operator=(TypedArray&& other);
    PLY_DLL_ENTRY void destroy();
    PLY_DLL_ENTRY void create(TypeDescriptorOwner* typeOwner, u32 numItems);
    PLY_DLL_ENTRY void assign(TypeDescriptorOwner* typeOwner, void* items, u32 numItems);
    PLY_DLL_ENTRY AnyObject append();

    template <typename T>
    PLY_INLINE void operator=(Array<T>&& arr) {
        u32 numItems = arr.numItems();
        assign(TypeOwnerResolver<T>::get(), arr.release(), numItems);
    }

    template <typename T>
    PLY_INLINE const T* get() const {
        PLY_ASSERT(getTypeDescriptor<T>()->isEquivalentTo(m_typeOwner->getRootType()));
        return (T*) m_array.m_items;
    }

    template <typename T>
    PLY_INLINE ArrayView<T> getView() const {
        PLY_ASSERT(getTypeDescriptor<T>()->isEquivalentTo(m_typeOwner->getRootType()));
        return {(T*) m_array.m_items, m_array.m_numItems};
    }

    PLY_INLINE const void* getBytes() const {
        return m_array.m_items;
    }

    PLY_INLINE const void* getBytesEnd() const {
        return PLY_PTR_OFFSET(m_array.m_items,
                              m_array.m_numItems * m_typeOwner->getRootType()->fixedSize);
    }

    PLY_INLINE u32 numItems() const {
        return m_array.m_numItems;
    }

    PLY_INLINE AnyObject getItem(u32 i) const {
        PLY_ASSERT(i < m_array.m_numItems);
        return {PLY_PTR_OFFSET(m_array.m_items, m_typeOwner->getRootType()->fixedSize * i),
                m_typeOwner->getRootType()};
    }

    PLY_INLINE TypeDescriptor* getItemType() const {
        return m_typeOwner->getRootType();
    }
};

template <>
struct TypeDescriptorSpecializer<TypedArray> {
    static PLY_DLL_ENTRY ply::TypeDescriptor* get();
};

} // namespace ply

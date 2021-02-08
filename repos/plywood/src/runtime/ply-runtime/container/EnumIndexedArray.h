/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>

namespace ply {

template <typename T_, typename EnumType_>
class EnumIndexedArray {
public:
    typedef T_ T;
    typedef EnumType_ EnumType;

private:
    T m_items[(u32) EnumType::Count];

public:
    PLY_INLINE EnumIndexedArray() = default;
    PLY_INLINE EnumIndexedArray(const T& item) {
        for (u32 i = 0; i < (u32) EnumType::Count; i++) {
            m_items[i] = item;
        }
    }
    PLY_INLINE EnumIndexedArray(std::initializer_list<T> items) {
        PLY_ASSERT(items.size() == (u32) EnumType::Count);
        for (u32 i = 0; i < (u32) EnumType::Count; i++) {
            m_items[i] = items[i];
        }
    }
    PLY_INLINE u32 numItems() const {
        return (u32) EnumType::Count;
    }
    PLY_INLINE T& operator[](u32 index) {
        PLY_ASSERT(index < (u32) EnumType::Count);
        return m_items[index];
    }
    PLY_INLINE const T& operator[](u32 index) const {
        PLY_ASSERT(index < (u32) EnumType::Count);
        return m_items[index];
    }
    PLY_INLINE T& operator[](EnumType index) {
        PLY_ASSERT(index < EnumType::Count);
        return m_items[(ureg) index];
    }
    PLY_INLINE const T& operator[](EnumType index) const {
        PLY_ASSERT(index < EnumType::Count);
        return m_items[(ureg) index];
    }
    PLY_INLINE ArrayView<const T> view() const {
        return {m_items, (u32) EnumType::Count};
    }
    PLY_INLINE ArrayView<T> view() {
        return {m_items, (u32) EnumType::Count};
    }
    PLY_INLINE operator ArrayView<const T>() const {
        return {m_items, (u32) EnumType::Count};
    }
    PLY_INLINE operator ArrayView<T>() {
        return {m_items, (u32) EnumType::Count};
    }
    PLY_INLINE T* begin() {
        return m_items;
    }
    PLY_INLINE T* end() {
        return m_items + (u32) EnumType::Count;
    }
    PLY_INLINE const T* begin() const {
        return m_items;
    }
    PLY_INLINE const T* end() const {
        return m_items + (u32) EnumType::Count;
    }
};

} // namespace ply

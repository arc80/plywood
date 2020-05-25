/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/container/ArrayView.h>

namespace ply {

namespace details {
template <typename T>
struct InitItems {
    static void init(T*) {
    }
    template <typename Arg, typename... RemainingArgs>
    static void init(T* items, Arg&& arg, RemainingArgs&&... remainingArgs) {
        *items = std::forward<Arg>(arg);
        init(items + 1, std::forward<RemainingArgs>(remainingArgs)...);
    }
};
} // namespace details

template <typename T, u32 Size>
struct FixedArray {
#if PLY_COMPILER_MSVC
#pragma warning(push)
#pragma warning(disable : 4200) // nonstandard extension used: zero-sized array in struct/union
#endif
    T items[Size];
#if PLY_COMPILER_MSVC
#pragma warning(pop)
#endif

    PLY_INLINE FixedArray() = default;

    template <typename... Args>
    PLY_INLINE FixedArray(Args&&... args) {
        PLY_STATIC_ASSERT(Size == sizeof...(Args));
        details::InitItems<T>::init(items, std::forward<Args>(args)...);
    }

    u32 numItems() const {
        return Size;
    }

    T& operator[](u32 i) {
        PLY_ASSERT(i < Size);
        return items[i];
    }

    const T& operator[](u32 i) const {
        PLY_ASSERT(i < Size);
        return items[i];
    }

    ArrayView<T> view() {
        return {items, Size};
    }

    ArrayView<const T> view() const {
        return {items, Size};
    }

    BufferView bufferView() {
        return {items, safeDemote<u32>(Size * sizeof(T))};
    }

    ConstBufferView bufferView() const {
        return {items, safeDemote<u32>(Size * sizeof(T))};
    }

    T* begin() {
        return items;
    }

    T* end() {
        return items + Size;
    }

    const T* begin() const {
        return items;
    }

    const T* end() const {
        return items + Size;
    }
};

} // namespace ply

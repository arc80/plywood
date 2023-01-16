/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/container/ArrayView.h>

namespace ply {

namespace impl {
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
} // namespace impl

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

    PLY_INLINE FixedArray(InitList<T> args) {
        PLY_ASSERT(Size == args.size());
        subst::constructArrayFrom(this->items, args.begin(), Size);
    }

    template <typename... Args>
    PLY_INLINE FixedArray(Args&&... args) {
        PLY_STATIC_ASSERT(Size == sizeof...(Args));
        impl::InitItems<T>::init(items, std::forward<Args>(args)...);
    }

    PLY_INLINE constexpr u32 numItems() const {
        return Size;
    }

    PLY_INLINE T& operator[](u32 i) {
        PLY_ASSERT(i < Size);
        return items[i];
    }

    PLY_INLINE const T& operator[](u32 i) const {
        PLY_ASSERT(i < Size);
        return items[i];
    }

    PLY_INLINE ArrayView<T> view() {
        return {items, Size};
    }

    PLY_INLINE ArrayView<const T> view() const {
        return {items, Size};
    }

    PLY_INLINE operator ArrayView<T>() {
        return {items, Size};
    }

    PLY_INLINE operator ArrayView<const T>() const {
        return {items, Size};
    }

    PLY_INLINE MutStringView mutableStringView() {
        return {reinterpret_cast<char*>(items), safeDemote<u32>(Size * sizeof(T))};
    }

    PLY_INLINE StringView stringView() const {
        return {reinterpret_cast<const char*>(items), safeDemote<u32>(Size * sizeof(T))};
    }

    PLY_INLINE T* begin() {
        return items;
    }

    PLY_INLINE T* end() {
        return items + Size;
    }

    PLY_INLINE const T* begin() const {
        return items;
    }

    PLY_INLINE const T* end() const {
        return items + Size;
    }
};

namespace impl {
template <typename T, u32 Size>
struct InitListType<FixedArray<T, Size>> {
    using Type = ArrayView<const T>;
};

template <typename T, u32 Size>
struct ArrayTraits<FixedArray<T, Size>> {
    using ItemType = T;
    static constexpr bool IsOwner = true;
};
} // namespace impl

} // namespace ply

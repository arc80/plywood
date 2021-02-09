/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/container/ArrayView.h>

namespace ply {

PLY_SFINAE_EXPR_2(IsComparable, std::declval<T0>() == std::declval<T1>());
PLY_SFINAE_EXPR_2(IsCallable, std::declval<T0>()(std::declval<T1>()));

// find
template <typename T, typename U, std::enable_if_t<IsComparable<T, U>, int> = 0>
PLY_INLINE s32 find(ArrayView<const T> arr, const U& item) {
    for (u32 i = 0; i < arr.numItems; i++) {
        if (arr[i] == item)
            return i;
    }
    return -1;
}

template <typename T, typename Callback, std::enable_if_t<IsCallable<Callback, T>, int> = 0>
PLY_INLINE s32 find(ArrayView<const T> arr, const Callback& callback) {
    for (u32 i = 0; i < arr.numItems; i++) {
        if (callback(arr[i]))
            return i;
    }
    return -1;
}

template <typename Arr, typename Arg, typename = details::ArrayViewType<Arr>>
PLY_INLINE s32 find(const Arr& arr, const Arg& arg) {
    return find(arr.view(), arg);
}

// rfind
template <typename T, typename U, std::enable_if_t<IsComparable<T, U>, int> = 0>
PLY_INLINE s32 rfind(ArrayView<const T> arr, const U& item) {
    for (s32 i = safeDemote<s32>(arr.numItems - 1); i >= 0; i--) {
        if (arr[i] == item)
            return i;
    }
    return -1;
}

template <typename T, typename Callback, std::enable_if_t<IsCallable<Callback, T>, int> = 0>
PLY_INLINE s32 rfind(ArrayView<const T> arr, const Callback& callback) {
    for (s32 i = safeDemote<s32>(arr.numItems - 1); i >= 0; i--) {
        if (callback(arr[i]))
            return i;
    }
    return -1;
}

template <typename Arr, typename Arg, typename = details::ArrayViewType<Arr>>
PLY_INLINE s32 rfind(const Arr& arr, const Arg& arg) {
    return rfind(arr.view(), arg);
}

} // namespace ply

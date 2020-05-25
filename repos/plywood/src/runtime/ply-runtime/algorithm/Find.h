/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/container/ArrayView.h>

namespace ply {

template <typename T, typename CompareExp>
PLY_INLINE s32 find(const ArrayView<T>& arr, const CompareExp& compareExp) {
    for (u32 i = 0; i < arr.numItems; i++) {
        if (compareExp(arr[i]))
            return i;
    }
    return -1;
}

template <typename T, typename U>
PLY_INLINE s32 findItem(const ArrayView<T>& arr, const U& item) {
    for (u32 i = 0; i < arr.numItems; i++) {
        if (arr[i] == item)
            return i;
    }
    return -1;
}

template <typename T, typename CompareExp>
PLY_INLINE s32 rfind(const ArrayView<T>& arr, const CompareExp& compareExp) {
    for (s32 i = safeDemote<s32>(arr.numItems - 1); i >= 0; i--) {
        if (compareExp(arr[i]))
            return i;
    }
    return -1;
}

template <typename T, typename U>
PLY_INLINE s32 rfindItem(const ArrayView<T>& arr, const U& item) {
    for (s32 i = safeDemote<s32>(arr.numItems - 1); i >= 0; i--) {
        if (arr[i] == item)
            return i;
    }
    return -1;
}

} // namespace ply

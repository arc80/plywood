/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/container/Array.h>

namespace ply {

template <typename T, typename FilterExp>
PLY_INLINE void filter(Array<T>& arr, const FilterExp& filterExp) {
    for (u32 i = 0; i < arr.numItems();) {
        if (filterExp(arr[i])) {
            i++;
        } else {
            arr.eraseQuick(i);
        }
    }
}

} // namespace ply

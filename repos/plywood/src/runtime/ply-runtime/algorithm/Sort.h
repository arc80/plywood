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
PLY_INLINE bool defaultLess(const T& a, const T& b) {
    return a < b;
}
} // namespace details

template <typename T, typename IsLess = decltype(details::defaultLess<T>)>
PLY_NO_INLINE void sort(ArrayView<T> view, const IsLess& isLess = details::defaultLess<T>) {
    if (view.numItems <= 1)
        return;
    u32 lo = 0;
    u32 hi = view.numItems - 1;
    u32 pivot = view.numItems / 2;
    for (;;) {
        while (lo < hi && isLess(view[lo], view[pivot])) {
            lo++;
        }
        while (lo < hi && isLess(view[pivot], view[hi])) {
            hi--;
        }
        if (lo >= hi)
            break;
        // view[lo] is >= pivot
        // All slots to left of lo are < pivot
        // view[hi] <= pivot
        // All slots to the right of hi are > pivot
        PLY_ASSERT(!isLess(view[lo], view[pivot]));
        PLY_ASSERT(!isLess(view[pivot], view[hi]));
        PLY_ASSERT(lo < hi);
        std::swap(view[lo], view[hi]);
        if (lo == pivot) {
            pivot = hi;
        } else if (hi == pivot) {
            pivot = lo;
        }
        lo++;
    }
    PLY_ASSERT((s32) hi >= 0);
    // Now, everything to left of lo is <= pivot, and everything from hi onwards is >= pivot.
    PLY_ASSERT(hi <= lo);
    while (lo > 1) {
        if (!isLess(view[lo - 1], view[pivot])) {
            lo--;
        } else {
            sort(view.subView(0, lo), isLess);
            break;
        }
    }
    while (hi + 1 < view.numItems) {
        if (!isLess(view[pivot], view[hi])) {
            hi++;
        } else {
            sort(view.subView(hi), isLess);
            break;
        }
    }
}

template <typename Arr,
          typename IsLess = decltype(details::defaultLess<details::ArrayViewType<Arr>>)>
PLY_INLINE void sort(Arr& arr,
                     const IsLess& isLess = details::defaultLess<details::ArrayViewType<Arr>>) {
    using T = details::ArrayViewType<Arr>;
    sort(ArrayView<T>{arr}, isLess);
}

} // namespace ply

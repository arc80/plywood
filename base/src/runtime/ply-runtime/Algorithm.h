/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                    ┃
┃    ╱   ╱╲    Plywood C++ Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/           ┃
┃    └──┴┴┴┘                                  ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/

#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/container/Array.h>
#include <ply-runtime/container/impl/ItemType.h>

namespace ply {

//    ▄▄▄ ▄▄ ▄▄▄   ▄▄
//   ██   ▄▄  ██  ▄██▄▄  ▄▄▄▄  ▄▄▄▄▄
//  ▀██▀  ██  ██   ██   ██▄▄██ ██  ▀▀
//   ██   ██ ▄██▄  ▀█▄▄ ▀█▄▄▄  ██
//

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

//    ▄▄▄ ▄▄            ▄▄
//   ██   ▄▄ ▄▄▄▄▄   ▄▄▄██
//  ▀██▀  ██ ██  ██ ██  ██
//   ██   ██ ██  ██ ▀█▄▄██
//

PLY_MAKE_WELL_FORMEDNESS_CHECK_2(IsComparable, std::declval<T0>() == std::declval<T1>());
PLY_MAKE_WELL_FORMEDNESS_CHECK_2(IsCallable, std::declval<T0>()(std::declval<T1>()));

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

template <typename Arr, typename Arg, typename T = impl::ArrayViewType<Arr>>
PLY_INLINE s32 find(const Arr& arr, const Arg& arg) {
    return find(ArrayView<const T>{arr}, arg);
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

template <typename Arr, typename Arg, typename T = impl::ArrayViewType<Arr>>
PLY_INLINE s32 rfind(const Arr& arr, const Arg& arg) {
    return rfind(ArrayView<const T>{arr}, arg);
}

//
//  ▄▄▄▄▄▄▄   ▄▄▄▄  ▄▄▄▄▄
//  ██ ██ ██  ▄▄▄██ ██  ██
//  ██ ██ ██ ▀█▄▄██ ██▄▄█▀
//                  ██

template <typename Iterable, typename MapFunc,
          typename MappedItemType = std::decay_t<
              decltype(std::declval<MapFunc>()(std::declval<impl::ItemType<Iterable>>()))>>
Array<MappedItemType> map(Iterable&& iterable, MapFunc&& mapFunc) {
    Array<MappedItemType> result;
    // FIXME: Reserve memory for result when possible. Otherwise, use a typed ChunkBuffer.
    for (auto&& item : iterable) {
        result.append(mapFunc(item));
    }
    return result;
}

//
//  ▄▄▄▄▄   ▄▄▄▄  ▄▄▄▄▄   ▄▄▄▄▄  ▄▄▄▄
//  ██  ▀▀  ▄▄▄██ ██  ██ ██  ██ ██▄▄██
//  ██     ▀█▄▄██ ██  ██ ▀█▄▄██ ▀█▄▄▄
//                        ▄▄▄█▀

struct Range {
    u32 i = 0;
    u32 limit = 0;

    struct Iterator {
        Range& range;
        PLY_INLINE u32 operator*() const {
            return range.i;
        }
        PLY_INLINE void operator++() {
            range.i++;
        }
        PLY_INLINE bool operator!=(const Iterator&) const {
            return range.i < range.limit;
        }
    };

    PLY_INLINE Iterator begin() {
        return {*this};
    }
    PLY_INLINE Iterator end() {
        return {*this};
    }
};

PLY_INLINE Range range(u32 limit) {
    return Range{0, limit};
}

PLY_INLINE Range range(u32 start, u32 limit) {
    return Range{start, limit};
}

//                    ▄▄
//  ▄▄▄▄▄   ▄▄▄▄   ▄▄▄██ ▄▄  ▄▄  ▄▄▄▄  ▄▄▄▄
//  ██  ▀▀ ██▄▄██ ██  ██ ██  ██ ██    ██▄▄██
//  ██     ▀█▄▄▄  ▀█▄▄██ ▀█▄▄██ ▀█▄▄▄ ▀█▄▄▄
//

template <typename Iterable, typename ReduceFunc>
impl::ItemType<Iterable> reduce(Iterable&& iterable, ReduceFunc&& reduceFunc,
                                const impl::ItemType<Iterable> initializer) {
    impl::ItemType<Iterable> result = initializer;
    for (auto&& item : iterable) {
        result = reduceFunc(std::move(result), item);
    }
    return result;
}

//                        ▄▄
//   ▄▄▄▄   ▄▄▄▄  ▄▄▄▄▄  ▄██▄▄
//  ▀█▄▄▄  ██  ██ ██  ▀▀  ██
//   ▄▄▄█▀ ▀█▄▄█▀ ██      ▀█▄▄
//

namespace impl {
template <typename T>
PLY_INLINE bool defaultLess(const T& a, const T& b) {
    return a < b;
}
} // namespace impl

template <typename T, typename IsLess = decltype(impl::defaultLess<T>)>
PLY_NO_INLINE void sort(ArrayView<T> view, const IsLess& isLess = impl::defaultLess<T>) {
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

template <typename Arr, typename IsLess = decltype(impl::defaultLess<impl::ArrayViewType<Arr>>)>
PLY_INLINE void sort(Arr& arr, const IsLess& isLess = impl::defaultLess<impl::ArrayViewType<Arr>>) {
    using T = impl::ArrayViewType<Arr>;
    sort(ArrayView<T>{arr}, isLess);
}

//
//   ▄▄▄▄  ▄▄  ▄▄ ▄▄▄▄▄▄▄
//  ▀█▄▄▄  ██  ██ ██ ██ ██
//   ▄▄▄█▀ ▀█▄▄██ ██ ██ ██
//

template <typename Iterable>
PLY_INLINE impl::ItemType<Iterable> sum(Iterable&& iterable) {
    impl::ItemType<Iterable> result = 0;
    for (auto&& item : iterable) {
        result = std::move(result) + item;
    }
    return result;
}

} // namespace ply

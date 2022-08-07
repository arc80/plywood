/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/container/details/ItemType.h>
#include <ply-runtime/container/Subst.h>

namespace ply {

template <typename Iterable, typename ReduceFunc>
details::ItemType<Iterable> reduce(Iterable&& iterable, ReduceFunc&& reduceFunc,
                                   const details::ItemType<Iterable> initializer) {
    details::ItemType<Iterable> result = initializer;
    for (auto&& item : iterable) {
        result = reduceFunc(std::move(result), item);
    }
    return result;
}

template <typename Iterable>
PLY_INLINE details::ItemType<Iterable> sum(Iterable&& iterable) {
    details::ItemType<Iterable> result = 0;
    for (auto&& item : iterable) {
        result = std::move(result) + item;
    }
    return result;
}

} // namespace ply

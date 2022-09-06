/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/container/Array.h>
#include <ply-runtime/container/impl/ItemType.h>

namespace ply {

template <typename Iterable, typename MapFunc,
          typename MappedItemType = std::decay_t<
              decltype(std::declval<MapFunc>()(std::declval<details::ItemType<Iterable>>()))>>
Array<MappedItemType> map(Iterable&& iterable, MapFunc&& mapFunc) {
    Array<MappedItemType> result;
    // FIXME: Reserve memory for result when possible. Otherwise, use a typed ChunkBuffer.
    for (auto&& item : iterable) {
        result.append(mapFunc(item));
    }
    return result;
}

} // namespace ply

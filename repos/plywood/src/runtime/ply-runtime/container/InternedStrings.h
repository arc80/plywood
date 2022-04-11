/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/string/StringView.h>
#include <ply-runtime/container/BigPool.h>
#include <ply-runtime/container/HashMap.h>
#include <ply-runtime/container/details/InternedStringEncoder.h>

namespace ply {

struct InternedStrings {
private:
    struct Traits {
        using Key = StringView;
        using Item = u32;
        using Context = BigPool<>;
        static bool match(u32 id, const StringView& key, const BigPool<>& bigPool) {
            const char* ptr = bigPool.get(id);
            u32 numBytes = details::InternedStringEncoder::decodeValue(ptr);
            return StringView{ptr, numBytes} == key;
        }
    };

    BigPool<> bigPool;
    HashMap<Traits> strToIndex;

public:
    InternedStrings();
    u32 findOrInsertKey(StringView view);
    StringView view(u32 key) const;
};

} // namespace ply

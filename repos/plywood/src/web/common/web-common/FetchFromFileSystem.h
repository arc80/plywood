/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <web-common/Core.h>
#include <web-common/Response.h>

namespace ply {
namespace web {

struct FetchFromFileSystem {
    struct ContentTypeTraits {
        using Key = StringView;
        struct Item {
            StringView extension;
            StringView mimeType;
            PLY_INLINE Item(StringView extension) : extension{extension} {
            }
        };
        PLY_INLINE static u32 hash(const Key& key) {
            String keyCopy = key; // FIXME: hack to satisfy hasher alignment
            Hasher hasher;
            keyCopy.appendTo(hasher);
            return hasher.result();
        }
        PLY_INLINE static const Key& comparand(const Item& item) {
            return item.extension;
        }
    };

    String rootDir;
    HashMap<ContentTypeTraits> extensionToContentType;

    PLY_NO_INLINE FetchFromFileSystem();
    PLY_NO_INLINE static void serve(const FetchFromFileSystem* params, StringView requestPath,
                                    ResponseIface* responseIface);
};

} // namespace web
} // namespace ply

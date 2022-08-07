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
        static PLY_INLINE bool match(const Item& item, Key key) {
            return item.extension == key;
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

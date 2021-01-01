/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-web-serve-docs/Core.h>
#include <web-common/Response.h>
#include <web-documentation/Contents.h>

namespace ply {
namespace web {

struct DocServer {
    struct ContentsTraits {
        using Key = StringView;
        struct Item {
            StringView linkPath;
            Contents* node = nullptr;
            PLY_INLINE Item(StringView linkPath) : linkPath{linkPath} {
            }
        };
        static PLY_INLINE Key comparand(const Item& item) {
            return item.linkPath;
        }
    };

    String dataRoot;
    String contentsPath;

    // These members are protected by contentsMutex:
    Mutex contentsMutex;
    Atomic<double> contentsModTime = 0;
    Array<Owned<Contents>> contents;
    HashMap<ContentsTraits> pathToContents;

    void init(StringView dataRoot);
    void reloadContents();
    void serve(const StringView requestPath, ResponseIface* responseIface);
    void serveContentOnly(const StringView requestPath, ResponseIface* responseIface);
};

} // namespace web
} // namespace ply

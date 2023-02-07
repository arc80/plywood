/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
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
            StringView link_path;
            Contents* node = nullptr;
            Item(StringView link_path) : link_path{link_path} {
            }
        };
        static bool match(const Item& item, Key key) {
            return item.link_path == key;
        }
    };

    String data_root;
    String contents_path;

    // These members are protected by contents_mutex:
    Mutex contents_mutex;
    Atomic<double> contents_mod_time = 0;
    Array<Owned<Contents>> contents;
    HashMap<ContentsTraits> path_to_contents;

    void init(StringView data_root);
    void reload_contents();
    void serve(StringView request_path, ResponseIface* response_iface);
    void serve_content_only(StringView request_path, ResponseIface* response_iface);
};

} // namespace web
} // namespace ply

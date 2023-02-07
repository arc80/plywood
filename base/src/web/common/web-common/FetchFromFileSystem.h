/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
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
            StringView mime_type;
            Item(StringView extension) : extension{extension} {
            }
        };
        static bool match(const Item& item, Key key) {
            return item.extension == key;
        }
    };

    String root_dir;
    HashMap<ContentTypeTraits> extension_to_content_type;

    FetchFromFileSystem();
    static void serve(const FetchFromFileSystem* params, StringView request_path,
                      ResponseIface* response_iface);
};

} // namespace web
} // namespace ply

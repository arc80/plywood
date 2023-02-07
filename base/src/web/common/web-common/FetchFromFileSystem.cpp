/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <web-common/Core.h>
#include <web-common/FetchFromFileSystem.h>

namespace ply {
namespace web {

FetchFromFileSystem::FetchFromFileSystem() {
    struct Pair {
        const char* key;
        const char* value;
    };
    for (const Pair& pair : {Pair{".woff2", "font/woff2"},
                             {".woff", "font/woff"},
                             {".ttf", "font/ttf"},
                             {".eot", "application/vnd.ms-fontobject"},
                             {".png", "image/png"},
                             {".css", "text/css"},
                             {".svg", "image/svg+xml"},
                             {".js", "text/javascript"}}) {
        auto cursor = this->extension_to_content_type.insert_or_find(pair.key);
        cursor->mime_type = pair.value;
    }
}

void FetchFromFileSystem::serve(const FetchFromFileSystem* params,
                                StringView request_path,
                                ResponseIface* response_iface) {
    s32 get_pos = request_path.find_byte('?');
    if (get_pos >= 0) {
        request_path = request_path.sub_str(0, get_pos);
    }

    String filename = Path.split(request_path).second;
    s32 dot_pos = filename.find_byte('.');
    if (dot_pos <= 0) {
        // no file extension
        response_iface->respond_generic(ResponseCode::NotFound);
        return;
    }

    auto cursor = params->extension_to_content_type.find(filename.sub_str(dot_pos));
    if (!cursor.was_found()) {
        // unrecognized file extension
        response_iface->respond_generic(ResponseCode::NotFound);
        return;
    }

    String native_path = Path.join(params->root_dir,
                                   request_path.ltrim([](char c) { return c == '/'; }));

    // FIXME: Don't load the whole file completely in memory first.
    // Could open the raw pipe and feed it to outs.
    String bin = FileSystem.load_binary(native_path);
    if (FileSystem.last_result() != FSResult::OK) {
        // file could not be loaded
        response_iface->respond_generic(ResponseCode::NotFound);
        return;
    }

    OutStream* outs = response_iface->begin_response_header(ResponseCode::OK);
    outs->format("Content-Type: {}\r\n", cursor->mime_type);
    *outs << "Cache-Control: max-age=1200\r\n\r\n";
    response_iface->end_response_header();
    outs->write(bin);
}

} // namespace web
} // namespace ply

/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once
#include <web-common/Core.h>

namespace ply {
namespace web {

enum class ResponseCode {
    Unknown = 0,
    OK,
    BadRequest,
    NotFound,
    InternalError,
};

struct Request {
    struct StartLine {
        StringView method;
        StringView uri;
        StringView http_version;
    };

    struct HeaderField {
        StringView name;
        StringView value;
    };

    IPAddress client_addr;
    u16 client_port = 0;
    StartLine start_line;
    Array<HeaderField> header_fields;
};

// This interface exists so that the same response code can be used both from FastCGI or
// from a webserver directly.
struct ResponseIface {
    Request request;

    // The request handler must call respond_with first, then manually write any
    // optional headers, followed by a blank \r\n line, followed by the content.
    virtual OutStream* begin_response_header(ResponseCode response_code) = 0;
    virtual void end_response_header() = 0;
    void respond_generic(ResponseCode response_code);
};

using RequestHandler =
    Functor<void(StringView request_path, ResponseIface* response_iface)>;

} // namespace web
} // namespace ply

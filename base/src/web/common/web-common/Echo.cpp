/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <web-common/Core.h>
#include <web-common/Response.h>

extern char** environ;

namespace ply {
namespace web {

void echo_serve(const void*, StringView request_path, ResponseIface* response_iface) {
    OutStream* outs = response_iface->begin_response_header(ResponseCode::OK);
    *outs << "Content-type: text/html\r\n\r\n";
    response_iface->end_response_header();
    *outs << R"(<html>
<head><title>Echo</title></head>
<body>
<center><h1>Echo</h1></center>
)";

    // Write client IP
    const auto& req = response_iface->request;
    outs->format("<p>Connection from: <code>{}:{}</code></p>",
                 req.client_addr.to_string(), req.client_port);

    // Write request header
    *outs << "<p>Request header:</p>\n";
    *outs << "<pre>\n";
    outs->format("{} {} {}\n", fmt::XMLEscape{req.start_line.method},
                 fmt::XMLEscape{req.start_line.uri},
                 fmt::XMLEscape{req.start_line.http_version});
    for (const Request::HeaderField& field : req.header_fields) {
        outs->format("{}: {}\n", fmt::XMLEscape{field.name},
                     fmt::XMLEscape{field.value});
    }
    *outs << "</pre>\n";

    // Write environment variables
    *outs << "<p>Environment variables:</p>\n";
    *outs << "<pre>\n";
    for (char** envp = environ; *envp != NULL; envp++) {
        *outs << fmt::XMLEscape{*envp} << "\n";
    }
    *outs << "</pre>\n";
    *outs << R"(</body>
</html>
)";
}

} // namespace web
} // namespace ply

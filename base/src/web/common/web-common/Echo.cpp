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

PLY_NO_INLINE void echo_serve(const void*, StringView requestPath, ResponseIface* responseIface) {
    OutStream* outs = responseIface->beginResponseHeader(ResponseCode::OK);
    *outs << "Content-type: text/html\r\n\r\n";
    responseIface->endResponseHeader();
    *outs << R"(<html>
<head><title>Echo</title></head>
<body>
<center><h1>Echo</h1></center>
)";

    // Write client IP
    const auto& req = responseIface->request;
    outs->format("<p>Connection from: <code>{}:{}</code></p>", req.clientAddr.toString(),
               req.clientPort);

    // Write request header
    *outs << "<p>Request header:</p>\n";
    *outs << "<pre>\n";
    outs->format("{} {} {}\n", fmt::XMLEscape{req.startLine.method},
               fmt::XMLEscape{req.startLine.uri}, fmt::XMLEscape{req.startLine.httpVersion});
    for (const Request::HeaderField& field : req.headerFields) {
        outs->format("{}: {}\n", fmt::XMLEscape{field.name}, fmt::XMLEscape{field.value});
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

/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <web-common/Core.h>
#include <web-common/Response.h>

extern char** environ;

namespace ply {
namespace web {

PLY_NO_INLINE void echo_serve(const void*, StringView requestPath, ResponseIface* responseIface) {
    StringWriter* sw = responseIface->respondWithStream(ResponseCode::OK)->strWriter();
    *sw << "Content-type: text/html\r\n\r\n";
    *sw << R"(<html>
<head><title>Echo</title></head>
<body>
<center><h1>Echo</h1></center>
)";

    // Write client IP
    const auto& req = responseIface->request;
    sw->format("<p>Connection from: <code>{}:{}</code></p>", req.clientAddr.toString(),
               req.clientPort);

    // Write request header
    *sw << "<p>Request header:</p>\n";
    *sw << "<pre>\n";
    sw->format("{} {} {}\n", fmt::XMLEscape{req.startLine.method},
               fmt::XMLEscape{req.startLine.uri}, fmt::XMLEscape{req.startLine.httpVersion});
    for (const Request::HeaderField& field : req.headerFields) {
        sw->format("{}: {}\n", fmt::XMLEscape{field.name}, fmt::XMLEscape{field.value});
    }
    *sw << "</pre>\n";

    // Write environment variables
    *sw << "<p>Environment variables:</p>\n";
    *sw << "<pre>\n";
    for (char** envp = environ; *envp != NULL; envp++) {
        *sw << fmt::XMLEscape{*envp} << "\n";
    }
    *sw << "</pre>\n";
    *sw << R"(</body>
</html>
)";
}

} // namespace web
} // namespace ply

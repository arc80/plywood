/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
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
        StringView httpVersion;
    };

    struct HeaderField {
        StringView name;
        StringView value;
    };

    IPAddress clientAddr;
    u16 clientPort = 0;
    StartLine startLine;
    Array<HeaderField> headerFields;
};

// This interface exists so that the same response code can be used both from FastCGI or from a
// webserver directly.
struct ResponseIface {
    Request request;

    // The request handler must call respondWith first, then manually write any optional headers,
    // followed by a blank \r\n line, followed by the content.
    virtual OutStream* beginResponseHeader(ResponseCode responseCode) = 0;
    virtual void endResponseHeader() = 0;
    void respondGeneric(ResponseCode responseCode);
};

using RequestHandler = HiddenArgFunctor<void(const StringView requestPath, ResponseIface* responseIface)>;

} // namespace web
} // namespace ply

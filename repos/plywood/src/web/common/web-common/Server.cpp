/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <web-common/Core.h>
#include <web-common/Server.h>
#include <ply-runtime/io/InStream.h>
#include <ply-runtime/io/OutStream.h>

namespace ply {
namespace web {

struct ThreadParams {
    Owned<TCPConnection> tcpConn;
    RequestHandler reqHandler;
};

PLY_NO_INLINE Tuple<StringView, StringView> getResponseDescription(ResponseCode responseCode) {
    switch (responseCode) {
        case ResponseCode::OK:
            return {"200", "OK"};
        case ResponseCode::BadRequest:
            return {"400", "Bad Request"};
        case ResponseCode::NotFound:
            return {"404", "Not Found"};
        case ResponseCode::InternalError:
        default:
            return {"500", "Internal Server Error"};
    }
}

struct ResponseIface_WebServer : ResponseIface {
    OutStream* outs = nullptr;
    bool gotResponse = false;

    PLY_INLINE ResponseIface_WebServer(OutStream* outs) : outs{outs} {
    }
    virtual OutStream* respondWithStream(ResponseCode responseCode) override {
        this->gotResponse = true;
        Tuple<StringView, StringView> responseDesc = getResponseDescription(responseCode);
        this->outs->strWriter()->format("HTTP/1.1 {} {}\r\n", responseDesc.first,
                                        responseDesc.second);
        // FIXME: Handle ResponseCode::InternalError the same way we would handle a crash
        return this->outs;
    }
    PLY_NO_INLINE void handleMissingResponse() {
        if (!this->gotResponse) {
            this->respondGeneric(ResponseCode::InternalError);
        }
    }
};

void ResponseIface::respondGeneric(ResponseCode responseCode) {
    OutStream* outs = respondWithStream(responseCode);
    Tuple<StringView, StringView> responseDesc = getResponseDescription(responseCode);
    *outs->strWriter() << "Content-Type: text/html\r\n\r\n";
    outs->strWriter()->format(R"(<html>
<head><title>{} {}</title></head>
<body>
<center><h1>{} {}</h1></center>
<hr>
</body>
</html>
)",
                              responseDesc.first, responseDesc.second, responseDesc.first,
                              responseDesc.second);
}

void serverThreadEntry(const ThreadParams& params) {
    InStream ins = params.tcpConn->createInStream();
    OutStream outs = params.tcpConn->createOutStream();

    // Create responseIface
    ResponseIface_WebServer responseIface{&outs};
    responseIface.request.clientAddr = params.tcpConn->remoteAddress();
    responseIface.request.clientPort = params.tcpConn->remotePort();

    // Parse HTTP headers: Read input lines up until a blank one
    // FIXME: Limit the size of the header to something like 16KB, otherwise someone could take down
    // the server.
    Array<String> lines;
    for (;;) {
        String line = ins.asStringReader()->readString<fmt::Line>();
        if (!line && ins.atEOF()) {
            // Ill-formed request
            responseIface.respondGeneric(ResponseCode::BadRequest);
            return;
        }
        if (line.findByte([](char u) { return !isWhite(u); }) < 0)
            break; // Blank line
        lines.append(line);
    }
    if (lines.numItems() == 0)
        return; // Ill-formed request

    // Split the start line into tokens:
    // FIXME: Add ability to split by any whitespace instead of just splitByte
    Array<StringView> tokens = lines[0].rtrim(isWhite).splitByte(' ');
    if (tokens.numItems() != 3) {
        // Ill-formed request
        responseIface.respondGeneric(ResponseCode::BadRequest);
        return;
    }
    responseIface.request.startLine = {tokens[0], tokens[1], tokens[2]};

    // Split remaining lines into name/value pairs:
    for (u32 i = 1; i < lines.numItems(); i++) {
        // FIXME: Support unfolding https://tools.ietf.org/html/rfc822#section-3.1
        if (isWhite(lines[i][0]))
            continue;
        s32 colonPos = lines[i].findByte(':');
        if (colonPos < 0) {
            // Ill-formed request
            responseIface.respondGeneric(ResponseCode::BadRequest);
            return;
        }
        responseIface.request.headerFields.append(
            {lines[i].left(colonPos).rtrim(isWhite), lines[i].subStr(colonPos + 1).trim(isWhite)});
    }

    // Note: ins is still open, so in the future, we could continue reading past the HTTP
    // header to support POST requests and WebSockets.

    // Invoke request handler
    params.reqHandler(tokens[1], &responseIface);
    responseIface.handleMissingResponse();

    return;
}

bool runServer(u16 port, const RequestHandler& reqHandler) {
    TCPListener listener = Socket::bindTCP(port);
    if (!listener.isValid()) {
        StdErr::createStringWriter().format("Error: Can't bind to port {}\n", port);
        return false;
    }

    for (;;) {
        ThreadParams params;
        params.tcpConn = listener.accept();
        // FIXME: Return if port stopped listening

        params.reqHandler = reqHandler;
        // FIXME: Use a thread pool instead of spawning a thread for every request
        Thread{[params{std::move(params)}] { serverThreadEntry(params); }};
    }
    return true;
}

} // namespace web
} // namespace ply

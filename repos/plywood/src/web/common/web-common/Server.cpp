/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <web-common/Core.h>
#include <web-common/Server.h>
#include <ply-runtime/io/InStream.h>
#include <ply-runtime/io/OutStream.h>
#include <web-common/OutPipe_HTTPChunked.h>

namespace ply {
namespace web {

//-----------------------------------------------------------------------
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
    enum State { NoResponse, BeganResponse, EndedHeader };

    OutStream* outs = nullptr;
    State state = NoResponse;
    bool isChunked = false; // implies keep-alive
    Owned<OutStream> outsChunked;

    PLY_INLINE ResponseIface_WebServer(OutStream* outs) : outs{outs} {
    }
    virtual OutStream* beginResponseHeader(ResponseCode responseCode) override {
        // FIXME: Handle ResponseCode::InternalError the same way we would handle a crash
        this->state = BeganResponse;
        Tuple<StringView, StringView> responseDesc = getResponseDescription(responseCode);
        this->outs->format("HTTP/1.1 {} {}\r\n", responseDesc.first, responseDesc.second);
        if (isChunked) {
            *this->outs << "Transfer-Encoding: chunked\r\n"
                           "Connection: keep-alive\r\n";
            outsChunked =
                Owned<OutStream>::create(Owned<OutPipe_HTTPChunked>::create(borrow(this->outs)));
            return outsChunked;
        } else {
            return this->outs;
        }
    }
    virtual void endResponseHeader() {
        if (isChunked) {
            outsChunked->flushMem();
            outsChunked->outPipe->cast<OutPipe_HTTPChunked>()->setChunkMode(true);
        }
        this->state = EndedHeader;
    }
    // Returns true if response was well-formed and it's possible to send another response over the
    // same connection:
    PLY_NO_INLINE bool handleMissingResponse() {
        if (this->state == NoResponse) {
            this->respondGeneric(ResponseCode::InternalError);
            return true; // respondGeneric makes it a well-formed response
        } else {
            // FIXME: Log somewhere if this->state != EndedHeader
            return this->state == EndedHeader;
        }
    }
};

void ResponseIface::respondGeneric(ResponseCode responseCode) {
    OutStream* outs = this->beginResponseHeader(responseCode);
    Tuple<StringView, StringView> responseDesc = getResponseDescription(responseCode);
    *outs << "Content-Type: text/html\r\n\r\n";
    this->endResponseHeader();
    outs->format(R"(<html>
<head><title>{} {}</title></head>
<body>
<center><h1>{} {}</h1></center>
<hr>
</body>
</html>
)",
                 responseDesc.first, responseDesc.second, responseDesc.first, responseDesc.second);
}

void serverThreadEntry(const ThreadParams& params) {
    InStream ins = params.tcpConn->createInStream();
    OutStream outs = params.tcpConn->createOutStream();

    for (;;) {
        // Create responseIface
        ResponseIface_WebServer responseIface{&outs};
        responseIface.request.clientAddr = params.tcpConn->remoteAddress();
        responseIface.request.clientPort = params.tcpConn->remotePort();

        // Parse HTTP headers: Read input lines up until a blank one
        // FIXME: Limit the size of the header to something like 16KB, otherwise someone could take
        // down the server.
        Array<String> lines;
        for (;;) {
            String line = ins.readString<fmt::Line>();
            if (!line && ins.atEOF()) {
                if (!lines.isEmpty()) {
                    // Ill-formed request
                    responseIface.respondGeneric(ResponseCode::BadRequest);
                }
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
                {lines[i].left(colonPos).rtrim(isWhite),
                 lines[i].subStr(colonPos + 1).trim(isWhite)});
        }

        // FIXME: Decide isChunked/keep-alive based on HTTP request headers
        responseIface.isChunked = (responseIface.request.startLine.httpVersion == "HTTP/1.1");

        // Note: ins is still open, so in the future, we could continue reading past the HTTP
        // header to support POST requests and WebSockets.

        // Invoke request handler
        params.reqHandler(tokens[1], &responseIface);

        if (!responseIface.handleMissingResponse())
            return; // Close connection if unable to distinguish between responses
        if (!responseIface.isChunked)
            return; // Close connection if not keep-alive
    }
}

bool runServer(u16 port, const RequestHandler& reqHandler) {
    TCPListener listener = Socket::bindTCP(port);
    if (!listener.isValid()) {
        StdErr::text().format("Error: Can't bind to port {}\n", port);
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

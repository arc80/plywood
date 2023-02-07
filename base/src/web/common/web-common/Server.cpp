/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <web-common/Core.h>
#include <web-common/Server.h>
#include <ply-runtime/io/InStream.h>
#include <ply-runtime/io/OutStream.h>
#include <web-common/OutPipe_HTTPChunked.h>

namespace ply {
namespace web {

//-----------------------------------------------------------------------
struct ThreadParams {
    Owned<TCPConnection> tcp_conn;
    RequestHandler req_handler;
};

Tuple<StringView, StringView> get_response_description(ResponseCode response_code) {
    switch (response_code) {
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
    bool is_chunked = false; // implies keep-alive
    Owned<OutStream> outs_chunked;

    ResponseIface_WebServer(OutStream* outs) : outs{outs} {
    }
    virtual OutStream* begin_response_header(ResponseCode response_code) override {
        // FIXME: Handle ResponseCode::InternalError the same way we would handle a
        // crash
        this->state = BeganResponse;
        Tuple<StringView, StringView> response_desc =
            get_response_description(response_code);
        this->outs->format("HTTP/1.1 {} {}\r\n", response_desc.first,
                           response_desc.second);
        if (is_chunked) {
            *this->outs << "Transfer-Encoding: chunked\r\n"
                           "Connection: keep-alive\r\n";
            outs_chunked = Owned<OutStream>::create(
                Owned<OutPipe_HTTPChunked>::create(borrow(this->outs)));
            return outs_chunked;
        } else {
            return this->outs;
        }
    }
    virtual void end_response_header() override {
        if (is_chunked) {
            outs_chunked->flush_mem();
            outs_chunked->out_pipe->cast<OutPipe_HTTPChunked>()->set_chunk_mode(true);
        }
        this->state = EndedHeader;
    }
    // Returns true if response was well-formed and it's possible to send another
    // response over the same connection:
    bool handle_missing_response() {
        if (this->state == NoResponse) {
            this->respond_generic(ResponseCode::InternalError);
            return true; // respond_generic makes it a well-formed response
        } else {
            // FIXME: Log somewhere if this->state != EndedHeader
            return this->state == EndedHeader;
        }
    }
};

void ResponseIface::respond_generic(ResponseCode response_code) {
    OutStream* outs = this->begin_response_header(response_code);
    Tuple<StringView, StringView> response_desc =
        get_response_description(response_code);
    *outs << "Content-Type: text/html\r\n\r\n";
    this->end_response_header();
    outs->format(R"(<html>
<head><title>{} {}</title></head>
<body>
<center><h1>{} {}</h1></center>
<hr>
</body>
</html>
)",
                 response_desc.first, response_desc.second, response_desc.first,
                 response_desc.second);
}

void server_thread_entry(const ThreadParams& params) {
    InStream ins = params.tcp_conn->create_in_stream();
    OutStream outs = params.tcp_conn->create_out_stream();

    for (;;) {
        // Create response_iface
        ResponseIface_WebServer response_iface{&outs};
        response_iface.request.client_addr = params.tcp_conn->remote_address();
        response_iface.request.client_port = params.tcp_conn->remote_port();

        // Parse HTTP headers: Read input lines up until a blank one
        // FIXME: Limit the size of the header to something like 16KB, otherwise someone
        // could take down the server.
        Array<String> lines;
        for (;;) {
            String line = ins.read_string<fmt::Line>();
            if (!line && ins.at_eof()) {
                if (!lines.is_empty()) {
                    // Ill-formed request
                    response_iface.respond_generic(ResponseCode::BadRequest);
                }
                return;
            }
            if (line.find_byte([](char u) { return !is_white(u); }) < 0)
                break; // Blank line
            lines.append(line);
        }
        if (lines.num_items() == 0)
            return; // Ill-formed request

        // Split the start line into tokens:
        // FIXME: Add ability to split by any whitespace instead of just split_byte
        Array<StringView> tokens = lines[0].rtrim(is_white).split_byte(' ');
        if (tokens.num_items() != 3) {
            // Ill-formed request
            response_iface.respond_generic(ResponseCode::BadRequest);
            return;
        }
        response_iface.request.start_line = {tokens[0], tokens[1], tokens[2]};

        // Split remaining lines into name/value pairs:
        for (u32 i = 1; i < lines.num_items(); i++) {
            // FIXME: Support unfolding https://tools.ietf.org/html/rfc822#section-3.1
            if (is_white(lines[i][0]))
                continue;
            s32 colon_pos = lines[i].find_byte(':');
            if (colon_pos < 0) {
                // Ill-formed request
                response_iface.respond_generic(ResponseCode::BadRequest);
                return;
            }
            response_iface.request.header_fields.append(
                {lines[i].left(colon_pos).rtrim(is_white),
                 lines[i].sub_str(colon_pos + 1).trim(is_white)});
        }

        // FIXME: Decide is_chunked/keep-alive based on HTTP request headers
        response_iface.is_chunked =
            (response_iface.request.start_line.http_version == "HTTP/1.1");

        // Note: ins is still open, so in the future, we could continue reading past the
        // HTTP header to support POST requests and WebSockets.

        // Invoke request handler
        params.req_handler(tokens[1], &response_iface);

        if (!response_iface.handle_missing_response())
            return; // Close connection if unable to distinguish between responses
        if (!response_iface.is_chunked)
            return; // Close connection if not keep-alive
    }
}

bool run_server(u16 port, const RequestHandler& req_handler) {
    TCPListener listener = Socket::bind_tcp(port);
    if (!listener.is_valid()) {
        StdErr::text().format("Error: Can't bind to port {}\n", port);
        return false;
    }

    for (;;) {
        ThreadParams params;
        params.tcp_conn = listener.accept();
        // FIXME: Return if port stopped listening

        params.req_handler = req_handler;
        // FIXME: Use a thread pool instead of spawning a thread for every request
        Thread{[params{std::move(params)}] { server_thread_entry(params); }};
    }
    return true;
}

} // namespace web
} // namespace ply

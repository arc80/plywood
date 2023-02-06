/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <web-common/Server.h>
#include <ply-web-serve-docs/DocServer.h>
#include <WebServer/Config.h>
#include <web-common/FetchFromFileSystem.h>
#include <web-common/SourceCode.h>
#include <web-common/Echo.h>
#include <web-common/OutPipe_HTTPChunked.h>

using namespace ply;
using namespace web;

struct AllParams {
    DocServer docs;
    FetchFromFileSystem file_sys;
    SourceCode source_code;
};

void my_request_handler(AllParams* params, StringView request_path,
                        ResponseIface* response_iface) {
    if (request_path.starts_with("/static/")) {
        FetchFromFileSystem::serve(&params->file_sys, request_path, response_iface);
    } else if (request_path.starts_with("/file/")) {
        SourceCode::serve(&params->source_code, request_path.sub_str(6),
                          response_iface);
    } else if (request_path.rtrim([](char c) { return c == '/'; }) == "/echo") {
        echo_serve(nullptr, request_path, response_iface);
    } else if (request_path.starts_with("/docs/")) {
        params->docs.serve(request_path.sub_str(6), response_iface);
    } else if (request_path.starts_with("/content?path=/docs/")) {
        params->docs.serve_content_only(request_path.sub_str(20), response_iface);
    } else if (request_path == "/") {
        params->docs.serve("", response_iface);
    } else if (request_path == "/favicon.ico") {
        FetchFromFileSystem::serve(&params->file_sys, "/static/favicon@32x32.png",
                                   response_iface);
    } else {
        response_iface->respond_generic(ResponseCode::NotFound);
    }
}

void write_msg_and_exit(StringView msg) {
    OutStream std_err = StdErr::text();
    std_err << "Error: " << msg;
    if (!msg.ends_with("\n")) {
        std_err << '\n';
    }
    std_err.flush_mem();
    exit(1);
}

struct CommandLine {
    Array<StringView> args;
    u32 index = 1;

    CommandLine(int argc, char* argv[])
        : args{ArrayView<const char*>({(const char**) argv, (u32) argc})} {
    }

    StringView read_token() {
        if (this->index >= this->args.num_items())
            return {};
        return args[index++];
    }
};

int main(int argc, char* argv[]) {
    Socket::initialize(IPAddress::V6);
    String data_root;
    u16 port = 0;
    CommandLine cmd_line{argc, argv};
    while (StringView arg = cmd_line.read_token()) {
        if (arg.starts_with("-")) {
            if (arg == "-p") {
                StringView port_str = cmd_line.read_token();
                if (!port_str) {
                    write_msg_and_exit(
                        String::format("Expected port number after {}", arg));
                }
                u16 p = port_str.to<u16>();
                if (p == 0) {
                    write_msg_and_exit(
                        String::format("Invalid port number {}", port_str));
                }
                port = p;
            } else {
                write_msg_and_exit(String::format("Unrecognized option {}", arg));
            }
        } else {
            if (data_root) {
                write_msg_and_exit("Too many arguments");
            }
            if (!FileSystem.is_dir(arg)) {
                write_msg_and_exit(String::format("Can't access directory at {}", arg));
            }
            data_root = arg;
        }
    }
#if PLY_TARGET_POSIX
    if (port == 0) {
        if (const char* port_cstr = getenv("PORT")) {
            u16 p = StringView{port_cstr}.to<u16>();
            if (p == 0) {
                write_msg_and_exit(String::format(
                    "Invalid port number {} found in environment variable PORT",
                    port_cstr));
            }
        }
    }
    if (port == 0) {
        if (const char* data_root_cstr = getenv("WEBSERVER_DOC_DIR")) {
            data_root = data_root_cstr;
        }
    }
#endif
    if (port == 0) {
        port = WEBSERVER_DEFAULT_PORT;
    }
    if (!data_root) {
        data_root = WEBSERVER_DEFAULT_DOC_DIR;
    }
    StdOut::text().format("Serving from {} on port {}\n", data_root, port);
    AllParams all_params;
    all_params.file_sys.root_dir = data_root;
    all_params.docs.init(data_root);
    all_params.source_code.root_dir = Path.normalize(Workspace.path);
    if (!run_server(port, {&all_params, my_request_handler})) {
        exit(1);
    }
    Socket::shutdown();
    return 0;
}

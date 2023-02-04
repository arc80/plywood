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
    FetchFromFileSystem fileSys;
    SourceCode sourceCode;
};

void myRequestHandler(AllParams* params, StringView requestPath, ResponseIface* responseIface) {
    if (requestPath.startsWith("/static/")) {
        FetchFromFileSystem::serve(&params->fileSys, requestPath, responseIface);
    } else if (requestPath.startsWith("/file/")) {
        SourceCode::serve(&params->sourceCode, requestPath.subStr(6), responseIface);
    } else if (requestPath.rtrim([](char c) { return c == '/'; }) == "/echo") {
        echo_serve(nullptr, requestPath, responseIface);
    } else if (requestPath.startsWith("/docs/")) {
        params->docs.serve(requestPath.subStr(6), responseIface);
    } else if (requestPath.startsWith("/content?path=/docs/")) {
        params->docs.serveContentOnly(requestPath.subStr(20), responseIface);
    } else if (requestPath == "/") {
        params->docs.serve("", responseIface);
    } else if (requestPath == "/favicon.ico") {
        FetchFromFileSystem::serve(&params->fileSys, "/static/favicon@32x32.png", responseIface);
    } else {
        responseIface->respondGeneric(ResponseCode::NotFound);
    }
}

void writeMsgAndExit(StringView msg) {
    OutStream stdErr = StdErr::text();
    stdErr << "Error: " << msg;
    if (!msg.endsWith("\n")) {
        stdErr << '\n';
    }
    stdErr.flushMem();
    exit(1);
}

struct CommandLine {
    Array<StringView> args;
    u32 index = 1;

    CommandLine(int argc, char* argv[])
        : args{ArrayView<const char*>({(const char**) argv, (u32) argc})} {
    }

    StringView readToken() {
        if (this->index >= this->args.numItems())
            return {};
        return args[index++];
    }
};

int main(int argc, char* argv[]) {
    Socket::initialize(IPAddress::V6);
    String dataRoot;
    u16 port = 0;
    CommandLine cmdLine{argc, argv};
    while (StringView arg = cmdLine.readToken()) {
        if (arg.startsWith("-")) {
            if (arg == "-p") {
                StringView portStr = cmdLine.readToken();
                if (!portStr) {
                    writeMsgAndExit(String::format("Expected port number after {}", arg));
                }
                u16 p = portStr.to<u16>();
                if (p == 0) {
                    writeMsgAndExit(String::format("Invalid port number {}", portStr));
                }
                port = p;
            } else {
                writeMsgAndExit(String::format("Unrecognized option {}", arg));
            }
        } else {
            if (dataRoot) {
                writeMsgAndExit("Too many arguments");
            }
            if (!FileSystem.isDir(arg)) {
                writeMsgAndExit(String::format("Can't access directory at {}", arg));
            }
            dataRoot = arg;
        }
    }
#if PLY_TARGET_POSIX
    if (port == 0) {
        if (const char* portCStr = getenv("PORT")) {
            u16 p = StringView{portCStr}.to<u16>();
            if (p == 0) {
                writeMsgAndExit(String::format(
                    "Invalid port number {} found in environment variable PORT", portCStr));
            }
        }
    }
    if (port == 0) {
        if (const char* dataRootCStr = getenv("WEBSERVER_DOC_DIR")) {
            dataRoot = dataRootCStr;
        }
    }
#endif
    if (port == 0) {
        port = WEBSERVER_DEFAULT_PORT;
    }
    if (!dataRoot) {
        dataRoot = WEBSERVER_DEFAULT_DOC_DIR;
    }
    StdOut::text().format("Serving from {} on port {}\n", dataRoot, port);
    AllParams allParams;
    allParams.fileSys.rootDir = dataRoot;
    allParams.docs.init(dataRoot);
    allParams.sourceCode.rootDir = Path.normalize(Workspace.path);
    if (!runServer(port, {&allParams, myRequestHandler})) {
        exit(1);
    }
    Socket::shutdown();
    return 0;
}

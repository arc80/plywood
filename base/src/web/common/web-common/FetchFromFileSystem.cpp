/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <web-common/Core.h>
#include <web-common/FetchFromFileSystem.h>

namespace ply {
namespace web {

PLY_NO_INLINE FetchFromFileSystem::FetchFromFileSystem() {
    struct Pair {
        const char* key;
        const char* value;
    };
    for (const Pair& pair : {Pair{".woff2", "font/woff2"},
                             {".woff", "font/woff"},
                             {".ttf", "font/ttf"},
                             {".eot", "application/vnd.ms-fontobject"},
                             {".png", "image/png"},
                             {".css", "text/css"},
                             {".svg", "image/svg+xml"},
                             {".js", "text/javascript"}}) {
        auto cursor = this->extensionToContentType.insertOrFind(pair.key);
        cursor->mimeType = pair.value;
    }
}

PLY_NO_INLINE void FetchFromFileSystem::serve(const FetchFromFileSystem* params,
                                              StringView requestPath,
                                              ResponseIface* responseIface) {
    s32 getPos = requestPath.findByte('?');
    if (getPos >= 0) {
        requestPath = requestPath.subStr(0, getPos);
    }

    String filename = NativePath::split(requestPath).second;
    s32 dotPos = filename.findByte('.');
    if (dotPos <= 0) {
        // no file extension
        responseIface->respondGeneric(ResponseCode::NotFound);
        return;
    }

    auto cursor = params->extensionToContentType.find(filename.subStr(dotPos));
    if (!cursor.wasFound()) {
        // unrecognized file extension
        responseIface->respondGeneric(ResponseCode::NotFound);
        return;
    }

    String nativePath =
        NativePath::join(params->rootDir, requestPath.ltrim([](char c) { return c == '/'; }));

    // FIXME: Don't load the whole file completely in memory first.
    // Could open the raw pipe and feed it to outs.
    String bin = FileSystem::native()->loadBinary(nativePath);
    if (FileSystem::native()->lastResult() != FSResult::OK) {
        // file could not be loaded
        responseIface->respondGeneric(ResponseCode::NotFound);
        return;
    }

    OutStream* outs = responseIface->beginResponseHeader(ResponseCode::OK);
    outs->format("Content-Type: {}\r\n", cursor->mimeType);
    *outs << "Cache-Control: max-age=1200\r\n\r\n";
    responseIface->endResponseHeader();
    outs->write(bin);
}

} // namespace web
} // namespace ply

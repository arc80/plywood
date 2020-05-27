/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-web-serve-docs/Core.h>
#include <web-common/Response.h>
#include <web-documentation/Contents.h>

namespace ply {
namespace web {

struct DocServer {
    String dataRoot;
    String contentsPath;

    // These members are protected by contentsMutex:
    Mutex contentsMutex;
    Atomic<double> contentsModTime = 0;
    Array<Contents> contents;

    void init(StringView dataRoot);
    void reloadContents();
    void serve(StringView requestPath, ResponseIface* responseIface);
};

} // namespace web
} // namespace ply

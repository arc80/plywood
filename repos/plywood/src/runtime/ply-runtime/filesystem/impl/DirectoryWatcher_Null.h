/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/filesystem/Path.h>

namespace ply {

class DirectoryWatcher_Null {
public:
    typedef void Callback(const Path& path, bool mustRecurse);

    DirectoryWatcher_Null(const Path&, const std::function<Callback>&) {
    }
};

} // namespace ply

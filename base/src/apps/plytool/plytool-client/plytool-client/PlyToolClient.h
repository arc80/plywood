/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <plytool-client/Core.h>
#include <plytool-client/Command.h>

namespace ply {

struct PlyToolClient {
    static void remoteRun(ArrayView<String> sourceFiles,
                          ArrayView<tool::Command::Dependency> dependencies);
};

} // namespace ply

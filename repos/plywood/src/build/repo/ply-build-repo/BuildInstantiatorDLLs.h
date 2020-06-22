/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-build-common/Core.h>
#include <ply-build-target/CMakeLists.h>

namespace ply {
namespace build {

struct InstantiatedDLL {
    String repoName;
    String dllPath;
};

Array<InstantiatedDLL> buildInstantiatorDLLs(bool force);

} // namespace build
} // namespace ply

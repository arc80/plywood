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

struct InstantiatedDLLs {
    u128 signature;
    Array<InstantiatedDLL> dlls;
};

InstantiatedDLLs buildInstantiatorDLLs(bool force);

} // namespace build
} // namespace ply

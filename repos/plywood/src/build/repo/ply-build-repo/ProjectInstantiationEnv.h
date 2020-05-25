/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-build-common/Core.h>
#include <ply-build-provider/ToolchainInfo.h>
#include <ply-build-repo/ExternProvider.h>

namespace ply {
namespace build {

struct ProjectInstantiationEnv {
    ToolchainInfo toolchain;
    Array<const ExternProvider*> externSelectors;
    String buildFolderPath;
    bool isValid = true;
};

} // namespace build
} // namespace ply

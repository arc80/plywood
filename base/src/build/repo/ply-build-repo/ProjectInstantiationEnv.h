/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-build-common/Core.h>
#include <ply-build-repo/ExternProvider.h>
#include <pylon/Node.h>

namespace ply {
namespace build {

struct CMakeGeneratorOptions;

struct ProjectInstantiationEnv {
    const CMakeGeneratorOptions* cmakeOptions = nullptr; // Used by the build-target module
    HybridString config;
    Owned<pylon::Node> toolchain;
    Array<const ExternProvider*> externSelectors;
    String buildFolderPath;
    bool isGenerating = false;
    bool isValid = true;
};

} // namespace build
} // namespace ply

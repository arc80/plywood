/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once
#include <ply-build-repo/Core.h>
#include <pylon/Node.h>

namespace ply {
namespace build {

struct CMakeGeneratorOptions {
    PLY_REFLECT()
    String generator;     // passed directly to -G
    String platform;      // passed directly to -A
    String toolset;       // passed directly to -T
    String toolchainFile; // currently "ios" or blank
    // ply reflect off

    template <typename Hasher>
    void appendTo(Hasher& h) const {
        h.append(this->generator);
        h.append(this->platform);
        h.append(this->toolset);
        h.append(this->toolchainFile);
    }
};

struct BuildFolder_t {
    String absPath;

    PLY_REFLECT()
    String solutionName;
    CMakeGeneratorOptions cmakeOptions;
    Array<String> rootTargets;
    Array<String> makeShared;
    Array<String> externSelectors;
    String activeConfig;
    String activeTarget;
    // ply reflect off

    // BuildFolder management
    bool load(StringView absPath);
    bool save() const;
    bool build(StringView config, StringView targetName) const;
};

} // namespace build
} // namespace ply

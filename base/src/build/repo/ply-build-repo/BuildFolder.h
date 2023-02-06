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
    String generator;      // passed directly to -G
    String platform;       // passed directly to -A
    String toolset;        // passed directly to -T
    String toolchain_file; // currently "ios" or blank
    // ply reflect off

    template <typename Hasher>
    void append_to(Hasher& h) const {
        h.append(this->generator);
        h.append(this->platform);
        h.append(this->toolset);
        h.append(this->toolchain_file);
    }
};

struct BuildFolder_t {
    String abs_path;

    PLY_REFLECT()
    String solution_name;
    CMakeGeneratorOptions cmake_options;
    Array<String> root_targets;
    Array<String> make_shared;
    Array<String> extern_selectors;
    String active_config;
    String active_target;
    // ply reflect off

    // BuildFolder management
    bool load(StringView abs_path);
    bool save() const;
    bool build(StringView config, StringView target_name) const;
};

} // namespace build
} // namespace ply

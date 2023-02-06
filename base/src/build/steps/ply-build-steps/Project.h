/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once
#include <ply-build-steps/Core.h>
#include <pylon/Node.h>

namespace ply {
namespace build {

PLY_INLINE bool has_all_bits(u64 bits_to_check, u64 desired_bits) {
    return (bits_to_check & desired_bits) == desired_bits;
}
PLY_INLINE bool has_bit_at_index(u64 bits_to_check, u32 index) {
    return (bits_to_check & (u64{1} << index)) != 0;
}

struct Option {
    enum Type {
        IncludeDir,
        PreprocessorDef,
        LinkerInput,
        Generic,
        CompilerSpecific,
        LinkerSpecific,
    };

    Type type = Type::IncludeDir;
    String key;
    String value;
    u64 enabled_bits = 0;   // whether option is enabled for each config
    u64 is_public_bits = 0; // whether option is public/private for each config

    PLY_INLINE Option(Type type, StringView key, StringView value = {})
        : type{type}, key{key}, value{value} {
    }
    PLY_INLINE bool operator==(const Option& other) const {
        return (this->type == other.type) && (this->key == other.key) &&
               (this->value == other.value);
    }
};

struct Target;

struct Dependency {
    Target* target = nullptr;
    u64 enabled_bits = 0;   // whether dependency is enabled for each config
    u64 is_public_bits = 0; // whether dependency is public/private for each config
};

struct SourceFile {
    String rel_path;
    u64 enabled_bits = 0; // whether path is enabled for each config
};

struct SourceGroup {
    String abs_path;
    Array<SourceFile> files;
};

struct Target {
    enum Type {
        Executable,
        Library,
        ObjectLibrary,
    };

    Label name;
    u64 enabled_bits = 0;        // ie. must be built and/or has a dependent, per-config
    u64 has_build_step_bits = 0; // ie. contains .cpp files, per-config
    Type type = Library;
    Array<Option> options;
    Array<Dependency> dependencies;
    Array<SourceGroup> source_groups;
    bool did_inheritance = false;

    PLY_INLINE void on_ref_count_zero() {
        delete this;
    }
};

struct CompilerSpecificOptions {
    Array<String> compile;
    Array<String> link;
};

extern void (*translate_toolchain_option)(CompilerSpecificOptions* copts,
                                          const Option& opt);

void init_toolchain_msvc();
void init_toolchain_gcc();

struct Project_ {
    String name;
    Array<String> config_names;
    Array<Option> per_config_options;
    Array<Owned<Target>> targets;
    bool did_inheritance = false;
};

extern Project_ Project;

void append_option(Array<Option>& options, const Option& src_opt);
void do_inheritance();
Array<Option> get_combined_options();
void write_CMakeLists_txt_if_different(StringView path);

} // namespace build
} // namespace ply

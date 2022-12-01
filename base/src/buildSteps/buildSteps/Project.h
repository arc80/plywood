/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <buildSteps/Core.h>
#include <pylon/Node.h>

namespace ply {
namespace build2 {

struct ConfigMask {
    u64 bits = 0;
    PLY_INLINE bool hasAllBitsIn(u64 mask) const {
        return (this->bits & mask) == mask;
    }
    PLY_INLINE bool hasAnyBit() const {
        return this->bits;
    }
    PLY_INLINE bool hasBitAtIndex(u32 idx) const {
        return (this->bits & (u64{1} << idx)) != 0;
    }
};

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
    ConfigMask enabled;  // whether option is enabled for each config
    ConfigMask isPublic; // whether option is public/private for each config

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
    ConfigMask enabled;  // whether dependency is enabled for each config
    ConfigMask isPublic; // whether dependency is public/private for each config
};

struct SourceFile {
    String relPath;
    ConfigMask enabled; // whether path is enabled for each config
};

struct SourceGroup {
    String absPath;
    Array<SourceFile> files;
};

struct Target {
    enum Type {
        Executable,
        Library,
    };

    Label name;
    ConfigMask enabled;      // ie. must be built and/or has a dependent, per-config
    ConfigMask hasBuildStep; // ie. contains .cpp files, per-config
    Type type = Library;
    Array<Option> options;
    Array<Dependency> dependencies;
    Array<SourceGroup> sourceGroups;
    bool didInheritance = false;

    PLY_INLINE void onRefCountZero() {
        delete this;
    }
};

struct CompilerSpecificOptions {
    Array<String> compile;
    Array<String> link;
};

extern void (*translate_toolchain_option)(CompilerSpecificOptions* copts, const Option& opt);

void init_toolchain_msvc();
void init_toolchain_gcc();

struct Project_ {
    String name;
    Array<String> configNames;
    Array<Option> perConfigOptions;
    Array<Owned<Target>> targets;
    bool didInheritance = false;
};

extern Project_ Project;

void do_inheritance();
Array<Option> get_combined_options();
void write_CMakeLists_txt_if_different(StringView path);

} // namespace build2
} // namespace ply

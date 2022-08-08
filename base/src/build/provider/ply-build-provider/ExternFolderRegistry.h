/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-build-common/Core.h>

namespace ply {
namespace build {

struct ExternFolder {
    struct Config {
        PLY_REFLECT()
        String args;
        bool success = false;
        // ply reflect off
    };

    String path; // Not written to the .pylon file

    PLY_REFLECT()
    String externName;
    String providerName;
    String folderArgs;
    bool success = false;
    Array<Config> multiConfig;
    // ply reflect off

    static Owned<ExternFolder> load(String&& path);
    PLY_BUILD_ENTRY bool save() const;
};

struct ExternFolderRegistry {
    Array<Owned<ExternFolder>> folders;

    static Owned<ExternFolderRegistry> instance_;
    static Owned<ExternFolderRegistry> create();
    static PLY_INLINE ExternFolderRegistry* get() {
        PLY_ASSERT(instance_);
        return instance_;
    }

    PLY_BUILD_ENTRY ExternFolder* find(StringView qualifiedName, StringView folderArgs) const;
};

} // namespace build
} // namespace ply

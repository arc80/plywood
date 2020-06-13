/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-build-common/Core.h>

namespace ply {
namespace build {

struct PackageManager;

struct HostTools {
    PLY_BUILD_ENTRY PackageManager* getApt() const;
    PLY_BUILD_ENTRY PackageManager* getConan() const;
    PLY_BUILD_ENTRY PackageManager* getMacPorts() const;
    PLY_BUILD_ENTRY PackageManager* getHomebrew() const;
    PLY_BUILD_ENTRY PackageManager* getVcpkg() const;

    PLY_BUILD_ENTRY static Owned<HostTools> instance_;

    static Owned<HostTools> create();
    static PLY_INLINE const HostTools* get() {
        PLY_ASSERT(instance_);
        return instance_;
    }
};

} // namespace build
} // namespace ply

/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-build-common/Core.h>

namespace ply {
namespace build {

struct PackageManager {
    virtual ~PackageManager() {
    }

    virtual bool isPackageInstalled(StringView packageName) = 0;
    // Note: A non-empty String returned from getInstallPrefix does not necessarily mean that the
    // package is installed:
    virtual String getInstallPrefix(StringView packageName) = 0;
    virtual bool installPackage(StringView packageName) = 0;
};

} // namespace build
} // namespace ply

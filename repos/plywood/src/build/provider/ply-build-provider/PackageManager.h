/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-build-common/Core.h>

namespace ply {
namespace build {

struct PackageManager {
    virtual bool isPackageInstalled(StringView packageName) = 0;
    virtual bool installPackage(StringView packageName) = 0;
};

} // namespace build
} // namespace ply

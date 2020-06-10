/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-build-common/Core.h>
#include <ply-build-repo/ExternProvider.h>

namespace ply {
namespace build {

struct PackageProvider {
    enum Manager {
        Apt,
        MacPorts,
        Homebrew,
    };

    Manager manager;
    String packageName;
    LambdaView<void(StringView prefix)> initDep;

    PLY_INLINE PackageProvider(Manager manager, StringView packageName,
                           LambdaView<void(StringView prefix)> initDep)
        : manager{manager}, packageName{packageName}, initDep{initDep} {
    }
    PLY_BUILD_ENTRY ExternResult handle(ExternCommand cmd, ExternProviderArgs* args);
};

} // namespace build
} // namespace ply

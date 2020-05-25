/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-build-common/Core.h>
#include <ply-build-provider/PackageManager.h>
#include <ply-build-provider/HostTools.h>

namespace ply {
namespace build {

struct PackageManager_Conan : PackageManager {
    virtual bool isPackageInstalled(StringView packageName) override {
        return false;
    }
    virtual bool installPackage(StringView packageName) override {
        return false;
    }
};

PLY_NO_INLINE PackageManager* HostTools::getConan() const {
    Owned<Subprocess> sub =
        Subprocess::exec("conan", {"--version"}, "", Subprocess::Output::openMerged());
    s32 rc = sub->join();
    PLY_UNUSED(rc);
    return nullptr;
}

} // namespace build
} // namespace ply

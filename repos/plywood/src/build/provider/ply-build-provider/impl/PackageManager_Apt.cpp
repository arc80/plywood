/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-build-common/Core.h>
#include <ply-build-provider/PackageManager.h>
#include <ply-build-provider/HostTools.h>

namespace ply {
namespace build {

struct PackageManager_Apt : PackageManager {
    virtual bool isPackageInstalled(StringView packageName) override {
        Owned<Subprocess> sub =
            Subprocess::exec("dpkg", {"-s", packageName}, {}, Subprocess::Output::ignore());
        return (sub && sub->join() == 0);
    }
    virtual bool installPackage(StringView packageName) override {
        StdOut::createStringWriter().format(
            R"(Warning: Superuser privileges are required to install '{}'.
If you're not comfortable entering your password here, cancel with Ctrl+C,
then run the following command manually:
    $ sudo apt-get install {}
)",
            packageName, packageName);
        Owned<Subprocess> sub =
            Subprocess::exec("sudo", {"-S", "apt-get", "install", packageName}, {},
                             Subprocess::Output::inherit(), Subprocess::Input::inherit());
        return (sub && sub->join() == 0);
    }
};

PLY_NO_INLINE PackageManager* HostTools::getApt() const {
    Owned<Subprocess> sub =
        Subprocess::exec("apt-get", {"--version"}, {}, Subprocess::Output::ignore());
    if (sub && sub->join() == 0) {
        static PackageManager_Apt apt;
        return &apt;
    }
    return nullptr;
}

} // namespace build
} // namespace ply

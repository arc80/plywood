/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-build-common/Core.h>
#include <ply-build-provider/PackageManager.h>
#include <ply-build-provider/HostTools.h>

namespace ply {
namespace build {

#if PLY_TARGET_WIN32
struct PackageManager_Vcpkg : PackageManager {
    bool isPackageInstalled(StringView packageName) override {
        Owned<Subprocess> sub = Subprocess::exec("vcpkg", {"list", packageName}, {},
                                                 Subprocess::Output::openStdOutOnly());
        if (!sub)
            return false;
        Owned<StringReader> sr = TextFormat::platformPreference().createImporter(
            Owned<InStream>::create(sub->readFromStdOut.borrow()));
        bool anyLines = false;
        while (String line = sr->readString<fmt::Line>()) {
            anyLines = true;
        }
        return (anyLines && sub->join() == 0);
    }

    String getInstallPrefix(StringView) override {
        return "$(VcpkgCurrentInstalledDir)";
    }

    bool installPackage(StringView packageName) override {
        Owned<Subprocess> sub =
            Subprocess::exec("vcpkg", {"install", packageName}, {}, Subprocess::Output::inherit(),
                             Subprocess::Input::inherit());
        return (sub && sub->join() == 0);
    }
};
#endif

PLY_NO_INLINE PackageManager* HostTools::getVcpkg() const {
#if PLY_TARGET_WIN32
    static Owned<PackageManager_Vcpkg> vcpkg = [] {
        Owned<Subprocess> sub =
            Subprocess::exec("vcpkg", {"version"}, {}, Subprocess::Output::ignore());
        return (sub && sub->join() == 0) ? new PackageManager_Vcpkg : nullptr;
    }();
    return vcpkg;
#else
    return nullptr;
#endif
}

} // namespace build
} // namespace ply

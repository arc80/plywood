/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-build-common/Core.h>
#include <ply-build-provider/PackageManager.h>
#include <ply-build-provider/HostTools.h>

namespace ply {
namespace build {

#if PLY_TARGET_APPLE
struct PackageManager_MacPorts : PackageManager {
    virtual bool isPackageInstalled(StringView packageName) override {
        Owned<Subprocess> sub = Subprocess::exec("port", {"-q", "installed", packageName}, {},
                                                 Subprocess::Output::openMerged());
        if (!sub)
            return false;
        Owned<StringReader> sr = TextFormat::platformPreference().createImporter(
            Owned<InStream>::create(sub->readFromStdOut.borrow()));
        bool found = false;
        while (String line = sr->readString<fmt::Line>()) {
            if (line.ltrim().startsWith(packageName))
                found = true;
        }
        return (found && sub->join() == 0);
    }

    virtual String getInstallPrefix(StringView packageName) override {
        // FIXME: Support MacPorts installations that install to a different prefix
        return "/opt/local";
    }

    virtual bool installPackage(StringView packageName) override {
        StdOut::createStringWriter().format(
            R"(Warning: Superuser privileges are required to install '{}'.
If you're not comfortable entering your password here, cancel with Ctrl+C,
then run the following command manually:
    $ sudo port install {}
)",
            packageName, packageName);
        Owned<Subprocess> sub =
            Subprocess::exec("sudo", {"-S", "port", "install", packageName}, {},
                             Subprocess::Output::inherit(), Subprocess::Input::inherit());
        return (sub && sub->join() == 0);
    }
};
#endif

PLY_NO_INLINE PackageManager* HostTools::getMacPorts() const {
#if PLY_TARGET_APPLE
    Owned<Subprocess> sub = Subprocess::exec("port", {"version"}, {}, Subprocess::Output::ignore());
    if (sub && sub->join() == 0) {
        static PackageManager_MacPorts macPorts;
        return &macPorts;
    }
#endif
    return nullptr;
}

} // namespace build
} // namespace ply

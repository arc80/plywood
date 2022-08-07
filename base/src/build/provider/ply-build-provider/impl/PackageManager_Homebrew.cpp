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
struct PackageManager_Homebrew : PackageManager {
    virtual bool isPackageInstalled(StringView packageName) override {
        Owned<Subprocess> sub = Subprocess::exec("brew", {"ls", "--versions", packageName}, {},
                                                 Subprocess::Output::openStdOutOnly());
        if (!sub)
            return false;
        Owned<InStream> ins = TextFormat::platformPreference().createImporter(
            Owned<InStream>::create(sub->readFromStdOut.borrow()));
        bool anyLines = false;
        while (String line = ins->readString<fmt::Line>()) {
            anyLines = true;
        }
        return (anyLines && sub->join() == 0);
    }

    virtual String getInstallPrefix(StringView packageName) override {
        Owned<Subprocess> sub = Subprocess::exec("brew", {"--prefix", packageName}, {},
                                                 Subprocess::Output::openStdOutOnly());
        if (!sub)
            return {};
        Owned<InStream> ins = TextFormat::platformPreference().createImporter(
            Owned<InStream>::create(sub->readFromStdOut.borrow()));
        String line = ins->readString<fmt::Line>();
        StringView prefix = line.trim();
        if (sub->join() != 0)
            return {};
        return prefix;
    }

    virtual bool installPackage(StringView packageName) override {
        Owned<Subprocess> sub =
            Subprocess::exec("brew", {"install", packageName}, {}, Subprocess::Output::inherit(),
                             Subprocess::Input::inherit());
        return (sub && sub->join() == 0);
    }
};
#endif

PLY_NO_INLINE PackageManager* HostTools::getHomebrew() const {
#if PLY_TARGET_APPLE
    static Owned<PackageManager_Homebrew> homebrew = [] {
        Owned<Subprocess> sub =
            Subprocess::exec("brew", {"--version"}, {}, Subprocess::Output::ignore());
        return (sub && sub->join() == 0) ? new PackageManager_Homebrew : nullptr;
    }();
    return homebrew;
#else
    return nullptr;
#endif
}

} // namespace build
} // namespace ply

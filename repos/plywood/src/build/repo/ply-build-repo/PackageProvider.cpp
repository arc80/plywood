/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-build-common/Core.h>
#include <ply-build-repo/PackageProvider.h>
#include <ply-build-provider/PackageManager.h>
#include <ply-build-provider/HostTools.h>

namespace ply {
namespace build {

PLY_NO_INLINE ExternResult PackageProvider::handle(ExternCommand cmd,
                                                          ExternProviderArgs* args) {
    if (args->providerArgs) {
        return {ExternResult::BadArgs, ""};
    }

    PackageManager* pkgMan = nullptr;
    if (this->manager == Manager::Apt) {
        pkgMan = HostTools::get()->getApt();
    } else if (this->manager == Manager::Homebrew) {
        pkgMan = HostTools::get()->getHomebrew();
    } else if (this->manager == Manager::MacPorts) {
        pkgMan = HostTools::get()->getMacPorts();
    }

    if (!pkgMan) {
        return {ExternResult::MissingPackageManager, {}};
    }

    ExternResult er = {pkgMan->isPackageInstalled(this->packageName)
                           ? ExternResult::Installed
                           : ExternResult::SupportedButNotInstalled,
                       ""};
    if (cmd == ExternCommand::Status) {
        return er;
    } else if (cmd == ExternCommand::Install) {
        if (er.code == ExternResult::Installed) {
            return er;
        }
        return {pkgMan->installPackage(this->packageName) ? ExternResult::Installed
                                                          : ExternResult::InstallFailed,
                ""};
    } else if (cmd == ExternCommand::Instantiate) {
        if (er.code != ExternResult::Installed) {
            return er;
        }
        String prefix = pkgMan->getInstallPrefix(this->packageName);
        this->initDep(prefix);
        return {ExternResult::Instantiated, ""};
    }
    PLY_ASSERT(0);
    return {ExternResult::Unknown, ""};
}

} // namespace build
} // namespace ply

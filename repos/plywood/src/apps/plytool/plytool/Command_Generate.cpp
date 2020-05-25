/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <Core.h>
#include <ConsoleUtils.h>
#include <ply-build-folder/BuildFolder.h>
#include <ply-build-repo/RepoRegistry.h>
#include <ply-build-repo/ProjectInstantiator.h>
#include <ply-build-provider/ExternFolderRegistry.h>
#include <ply-build-provider/HostTools.h>
#include <ply-build-provider/ToolchainInfo.h>

namespace ply {

bool command_generate(PlyToolCommandEnv* env) {
    using namespace build;
    if (!env->currentBuildFolder) {
        fatalError("Current build folder not set");
    }

    ensureTerminated(env->cl);
    env->cl->finalize();

    PLY_SET_IN_SCOPE(RepoRegistry::instance_, RepoRegistry::create(env->workspace->cmakeOptions));
    PLY_SET_IN_SCOPE(ExternFolderRegistry::instance_, ExternFolderRegistry::create());
    PLY_SET_IN_SCOPE(HostTools::instance_, HostTools::create());

    for (;;) {
        ProjectInstantiationResult instResult = env->currentBuildFolder->instantiateAllTargets();
        if (!instResult.isValid) {
            return false;
        }
        bool canGenerate = true;
        u32 numUnselected = instResult.unselectedExterns.numItems();
        if (numUnselected > 0) {
            canGenerate = false;
            StringWriter sw = StdOut::createStringWriter();
            for (const DependencySource* unselectedExtern : instResult.unselectedExterns) {
                sw.format("Can't generate build system in folder '{}' because extern '{}' is not "
                          "selected.\n",
                          env->currentBuildFolder->buildFolderName,
                          RepoRegistry::get()->getShortDepSourceName(unselectedExtern));
                Array<Tuple<const ExternProvider*, bool>> candidates;
                for (const Repo* repo : RepoRegistry::get()->repos) {
                    for (const ExternProvider* externProvider : repo->externProviders) {
                        if (externProvider->extern_ != unselectedExtern)
                            continue;
                        ToolchainInfo toolchain =
                            toolchainInfoFromCMakeOptions(env->currentBuildFolder->cmakeOptions);
                        ExternProviderArgs args;
                        args.toolchain = &toolchain;
                        args.provider = externProvider;
                        ExternResult er = externProvider->externFunc(ExternCommand::Status, &args);
                        if (er.isSupported()) {
                            candidates.append({externProvider, er.code == ExternResult::Installed});
                        }
                    }
                }
                if (candidates.isEmpty()) {
                    sw.format("No compatible providers are available for extern '{}'.\n",
                              RepoRegistry::get()->getShortDepSourceName(unselectedExtern));
                } else {
                    u32 n = candidates.numItems();
                    sw.format("{} compatible provider{} available:\n", n, n == 1 ? " is" : "s are");
                    for (Tuple<const ExternProvider*, bool> pair : candidates) {
                        sw.format("    {} ({})\n",
                                  RepoRegistry::get()->getShortProviderName(pair.first),
                                  pair.second ? "installed" : "not installed");
                    }
                }
            }
        }
        if (instResult.uninstalledProviders.numItems() > 0) {
            canGenerate = false;
            StringWriter sw = StdOut::createStringWriter();
            for (const ExternProvider* prov : instResult.uninstalledProviders) {
                sw.format("Can't generate build system in folder '{}' because extern provider "
                          "'{}' is selected, but not installed.\n",
                          env->currentBuildFolder->buildFolderName,
                          RepoRegistry::get()->getShortProviderName(prov));
            }
        }
        if (canGenerate) {
            if (env->currentBuildFolder->generate(&instResult)) {
                StdOut::createStringWriter().format(
                    "Successfully generated build system in folder '{}'.\n",
                    env->currentBuildFolder->buildFolderName);
                return true;
            }
        }
        break;
    }
    return false;
}

} // namespace ply

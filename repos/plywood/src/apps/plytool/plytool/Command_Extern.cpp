/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <Core.h>
#include <ConsoleUtils.h>
#include <ply-build-folder/BuildFolder.h>
#include <ply-build-repo/RepoRegistry.h>
#include <ply-build-repo/ProjectInstantiator.h>
#include <ply-build-provider/HostTools.h>
#include <ply-build-provider/ToolchainInfo.h>
#include <ply-build-provider/ExternFolderRegistry.h>

namespace ply {

void installProvider(PlyToolCommandEnv* env, const build::ExternProvider* provider) {
    using namespace build;
    PLY_SET_IN_SCOPE(ExternFolderRegistry::instance_, ExternFolderRegistry::create());
    PLY_SET_IN_SCOPE(HostTools::instance_, HostTools::create());

    StdOut::createStringWriter().format("Installing extern provider '{}'...\n",
                                        RepoRegistry::get()->getShortProviderName(provider));

    ToolchainInfo toolchain = toolchainInfoFromCMakeOptions(env->currentBuildFolder->cmakeOptions);
    ExternProviderArgs args;
    args.toolchain = &toolchain;
    args.provider = provider;
    ExternResult er = provider->externFunc(ExternCommand::Install, &args);
    String errorDetails;
    if (er.details) {
        errorDetails = String::format(" ({})", er.details);
    }
    switch (er.code) {
        case ExternResult::BadArgs: {
            fatalError(String::format("bad provider arguments{}\n", errorDetails));
            break;
        }
        case ExternResult::UnsupportedHost: {
            fatalError(String::format("can't install on host{}\n", errorDetails));
            break;
        }
        case ExternResult::MissingPackageManager: {
            fatalError(String::format("missing package manager{}\n", errorDetails));
            break;
        }
        case ExternResult::UnsupportedToolchain: {
            fatalError(String::format("toolchain is not supported{}\n", errorDetails));
            break;
        }
        case ExternResult::Installed: {
            StdOut::createStringWriter()
                << String::format("Successfully installed extern provider '{}'.\n",
                                  RepoRegistry::get()->getShortProviderName(provider));
            break;
        }
        case ExternResult::InstallFailed: {
            fatalError(String::format("install failed{}\n", errorDetails));
            break;
        }
        default: {
            fatalError("internal provider returned invalid code");
            break;
        }
    }
}

void command_extern(PlyToolCommandEnv* env) {
    using namespace build;
    if (!env->currentBuildFolder) {
        fatalError("Current build folder not set");
    }

    StringView cmd = env->cl->readToken();
    if (cmd.isEmpty()) {
        fatalError("Expected extern command");
    }

    if (prefixMatch(cmd, "list")) {
        ensureTerminated(env->cl);
        env->cl->finalize();

        PLY_SET_IN_SCOPE(RepoRegistry::instance_,
                         RepoRegistry::create(env->workspace->cmakeOptions));

        for (const Repo* repo : RepoRegistry::get()->repos) {
            StringWriter sw = StdOut::createStringWriter();
            u32 n = repo->externs.numItems();
            sw.format("{} extern{} defined in repo '{}'{}\n", n, n == 1 ? "" : "s", repo->repoName,
                      n == 0 ? "." : ":");
            for (const DependencySource* extern_ : repo->externs) {
                sw.format("    {}\n", RepoRegistry::get()->getShortDepSourceName(extern_));
            }
        }
    } else if (prefixMatch(cmd, "info")) {
        StringView externName = env->cl->readToken();
        if (externName.isEmpty()) {
            fatalError("Expected extern name");
        }
        ensureTerminated(env->cl);
        env->cl->finalize();

        PLY_SET_IN_SCOPE(RepoRegistry::instance_,
                         RepoRegistry::create(env->workspace->cmakeOptions));
        PLY_SET_IN_SCOPE(ExternFolderRegistry::instance_, ExternFolderRegistry::create());
        PLY_SET_IN_SCOPE(HostTools::instance_, HostTools::create());

        const DependencySource* extern_ = RepoRegistry::get()->findExtern(externName);
        if (!extern_) {
            fatalError(String::format("Can't find extern '{}'", externName));
        }

        Array<Tuple<const ExternProvider*, ExternResult::Code>> candidates;
        for (const Repo* repo : RepoRegistry::get()->repos) {
            for (const ExternProvider* externProvider : repo->externProviders) {
                if (externProvider->extern_ != extern_)
                    continue;
                ToolchainInfo toolchain =
                    toolchainInfoFromCMakeOptions(env->currentBuildFolder->cmakeOptions);
                ExternProviderArgs args;
                args.toolchain = &toolchain;
                args.provider = externProvider;
                ExternResult er = externProvider->externFunc(ExternCommand::Status, &args);
                candidates.append({externProvider, er.code});
            }
        }
        u32 n = candidates.numItems();
        StringWriter sw = StdOut::createStringWriter();
        sw.format("Found {} provider{} for extern '{}':\n", n, n == 1 ? "" : "s",
                  RepoRegistry::get()->getShortDepSourceName(extern_));
        for (Tuple<const ExternProvider*, ExternResult::Code> pair : candidates) {
            StringView codeStr = "???";
            switch (pair.second) {
                case ExternResult::Installed: {
                    codeStr = "installed";
                    break;
                }
                case ExternResult::SupportedButNotInstalled: {
                    codeStr = "not installed";
                    break;
                }
                case ExternResult::UnsupportedHost:
                case ExternResult::MissingPackageManager: {
                    codeStr = "not supported on host system";
                    break;
                }
                case ExternResult::UnsupportedToolchain: {
                    codeStr = "not compatible with build folder";
                    break;
                }
                default:
                    break;
            }
            sw.format("    {} ({})\n", RepoRegistry::get()->getShortProviderName(pair.first),
                      codeStr);
        }
    } else if (prefixMatch(cmd, "select")) {
        StringView qualifiedName = env->cl->readToken();
        if (qualifiedName.isEmpty()) {
            fatalError("Expected provider name");
        }
        ensureTerminated(env->cl);
        bool shouldInstall = env->cl->checkForSkippedOpt("--install");
        env->cl->finalize();

        PLY_SET_IN_SCOPE(RepoRegistry::instance_,
                         RepoRegistry::create(env->workspace->cmakeOptions));
        const ExternProvider* provider = RepoRegistry::get()->getExternProvider(qualifiedName);
        if (!provider) {
            fatalError("Provider not found");
        }

        if (shouldInstall) {
            installProvider(env, provider);
        }

        // Remove any selected providers that match the same extern
        for (u32 i = 0; i < env->currentBuildFolder->externSelectors.numItems();) {
            const ExternProvider* other =
                RepoRegistry::get()->getExternProvider(env->currentBuildFolder->externSelectors[i]);
            if (other && other->extern_ == provider->extern_) {
                env->currentBuildFolder->externSelectors.erase(i);
            } else {
                i++;
            }
        }

        env->currentBuildFolder->externSelectors.append(provider->getFullyQualifiedName());
        env->currentBuildFolder->save();

        StdOut::createStringWriter().format("Selected extern provider '{}' in build folder '{}'.\n",
                                            RepoRegistry::get()->getShortProviderName(provider),
                                            env->currentBuildFolder->buildFolderName);
    } else if (prefixMatch(cmd, "selected")) {
        ensureTerminated(env->cl);
        env->cl->finalize();

        StringWriter sw = StdOut::createStringWriter();
        sw.format("The following extern providers are selected in build folder '{}':\n",
                  env->currentBuildFolder->buildFolderName);
        for (StringView sel : env->currentBuildFolder->externSelectors) {
            sw.format("    {}\n", sel);
        }
    } else if (prefixMatch(cmd, "install")) {
        StringView qualifiedName = env->cl->readToken();
        if (qualifiedName.isEmpty()) {
            fatalError("Expected provider name");
        }
        ensureTerminated(env->cl);
        env->cl->finalize();

        PLY_SET_IN_SCOPE(RepoRegistry::instance_,
                         RepoRegistry::create(env->workspace->cmakeOptions));
        const ExternProvider* provider = RepoRegistry::get()->getExternProvider(qualifiedName);
        if (!provider) {
            fatalError("Provider not found");
        }

        installProvider(env, provider);
    } else {
        fatalError(String::format("Unrecognized extern command \"{}\"", cmd));
    }
}

} // namespace ply

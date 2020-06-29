#include <ply-build-repo/Module.h>

// [ply module="web-sass"]
void module_webSass(ModuleArgs* args) {
    args->addIncludeDir(Visibility::Public, "sass");
    args->addSourceFiles("sass/web-sass");
    args->addTarget(Visibility::Public, "runtime");
    args->addExtern(Visibility::Public, "libsass");
}

// [ply module="web-markdown"]
void module_webMarkdown(ModuleArgs* args) {
    args->addIncludeDir(Visibility::Public, "markdown");
    args->addSourceFiles("markdown/web-markdown");
    args->addTarget(Visibility::Public, "reflect");
    args->addTarget(Visibility::Private, "web-common");
}

// [ply module="web-common"]
void module_webCommon(ModuleArgs* args) {
    args->addIncludeDir(Visibility::Public, "common");
    args->addSourceFiles("common/web-common");
    args->addTarget(Visibility::Public, "runtime");
}

// [ply module="web-documentation"]
void module_webDocumentation(ModuleArgs* args) {
    args->addIncludeDir(Visibility::Public, "documentation");
    args->addSourceFiles("documentation/web-documentation");
    args->addTarget(Visibility::Public, "reflect");
}

// [ply module="web-cook-docs"]
void module_webCookDocs(ModuleArgs* args) {
    args->addIncludeDir(Visibility::Public, "cook-docs");
    args->addIncludeDir(Visibility::Public, NativePath::join(args->projInst->env->buildFolderPath,
                                                             "codegen/web-cook-docs"));
    args->addSourceFiles("cook-docs/ply-web-cook-docs");
    args->addTarget(Visibility::Public, "reflect");
    args->addTarget(Visibility::Private, "web-common");
    args->addTarget(Visibility::Private, "web-sass");
    args->addTarget(Visibility::Private, "web-markdown");
    args->addTarget(Visibility::Private, "cook");
    args->addTarget(Visibility::Private, "pylon-reflect");
    args->addTarget(Visibility::Private, "plytool-client");
    args->addTarget(Visibility::Private, "web-documentation");
    args->addTarget(Visibility::Public, "cpp");

    if (args->projInst->env->isGenerating) {
        StringView configFile = "#define WEBCOOKDOCS_LINK_TO_GITHUB 0\n";
        FileSystem::native()->makeDirsAndSaveTextIfDifferent(
            NativePath::join(args->projInst->env->buildFolderPath,
                             "codegen/web-cook-docs/ply-web-cook-docs/Config.h"),
            configFile, TextFormat::platformPreference());
    }
}

// [ply module="web-serve-docs"]
void module_webServeDocs(ModuleArgs* args) {
    args->addIncludeDir(Visibility::Public, "serve-docs");
    args->addSourceFiles("serve-docs/ply-web-serve-docs");
    args->addTarget(Visibility::Public, "runtime");
    args->addTarget(Visibility::Private, "pylon-reflect");
    args->addTarget(Visibility::Private, "web-documentation");
    args->addTarget(Visibility::Private, "web-common"); // for ResponseIface
}

// [ply extern="libsass" provider="macports"]
ExternResult extern_libsass_macports(ExternCommand cmd, ExternProviderArgs* args) {
    PackageProvider prov{PackageProvider::MacPorts, "libsass", [&](StringView prefix) {
                             args->dep->includeDirs.append(NativePath::join(prefix, "include"));
                             args->dep->libs.append(NativePath::join(prefix, "lib/libsass.a"));
                         }};
    return prov.handle(cmd, args);
}

// [ply extern="libsass" provider="homebrew"]
ExternResult extern_libsass_homebrew(ExternCommand cmd, ExternProviderArgs* args) {
    PackageProvider prov{PackageProvider::Homebrew, "libsass", [&](StringView prefix) {
                             args->dep->includeDirs.append(NativePath::join(prefix, "include"));
                             args->dep->libs.append(NativePath::join(prefix, "lib/libsass.a"));
                         }};
    return prov.handle(cmd, args);
}

// [ply extern="libsass" provider="apt"]
ExternResult extern_libsass_apt(ExternCommand cmd, ExternProviderArgs* args) {
    PackageProvider prov{PackageProvider::Apt, "libsass-dev",
                         [&](StringView) { args->dep->libs.append("-lsass"); }};
    return prov.handle(cmd, args);
}

// [ply extern="libsass" provider="prebuilt"]
ExternResult extern_libsass_prebuilt(ExternCommand cmd, ExternProviderArgs* args) {
    // Toolchain filters
    if (args->toolchain->get("targetPlatform")->text() != "windows") {
        return {ExternResult::UnsupportedToolchain, "Target platform must be 'windows'"};
    }
    StringView arch = args->toolchain->get("arch")->text();
    if (findItem(ArrayView<const StringView>{"x86", "x64"}, arch) < 0) {
        return {ExternResult::UnsupportedToolchain, "Target arch must be 'x86' or 'x64'"};
    }
    if (args->providerArgs) {
        return {ExternResult::BadArgs, ""};
    }

    StringView version = "3.6.3";
    String archiveName = String::format("libsass-{}-{}", version, arch);

    // Handle Command
    Tuple<ExternResult, ExternFolder*> er = args->findExistingExternFolder(arch);
    if (cmd == ExternCommand::Status) {
        return er.first;
    } else if (cmd == ExternCommand::Install) {
        if (er.first.code != ExternResult::SupportedButNotInstalled) {
            return er.first;
        }
        ExternFolder* externFolder = args->createExternFolder(arch);
        String archivePath = NativePath::join(externFolder->path, archiveName + ".msi");
        String url = String::format("https://github.com/sass/libsass/releases/download/{}/{}.msi",
                                    version, archiveName);
        if (!downloadFile(archivePath, url)) {
            return {ExternResult::InstallFailed, String::format("Error downloading '{}'", url)};
        }
        if (!extractFile(archivePath)) {
            return {ExternResult::InstallFailed,
                    String::format("Error extracting '{}'", archivePath)};
        }
        FileSystem::native()->deleteFile(archivePath);
        externFolder->success = true;
        externFolder->save();
        return {ExternResult::Installed, ""};
    } else if (cmd == ExternCommand::Instantiate) {
        if (er.first.code != ExternResult::Installed) {
            return er.first;
        }
        String installFolder = NativePath::join(er.second->path, archiveName, "libsass");
        args->dep->includeDirs.append(NativePath::join(installFolder, "include"));
        args->dep->libs.append(NativePath::join(installFolder, "libsass.a"));
        args->dep->dlls.append(NativePath::join(installFolder, "libsass.dll"));
        return {ExternResult::Instantiated, ""};
    }
    PLY_ASSERT(0);
    return {ExternResult::Unknown, ""};
}

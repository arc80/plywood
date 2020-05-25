// ply instantiate web-sass
void inst_webSass(TargetInstantiatorArgs* args) {
    args->addIncludeDir(Visibility::Public, "sass");
    args->addSourceFiles("sass/web-sass");
    args->addTarget(Visibility::Public, "runtime");
    args->addExtern(Visibility::Public, "libsass");
}

// ply instantiate web-markdown
void inst_webMarkdown(TargetInstantiatorArgs* args) {
    args->addIncludeDir(Visibility::Public, "markdown");
    args->addSourceFiles("markdown/web-markdown");
    args->addTarget(Visibility::Public, "reflect");
    args->addTarget(Visibility::Private, "web-common");
}

// ply instantiate web-common
void inst_webCommon(TargetInstantiatorArgs* args) {
    args->addIncludeDir(Visibility::Public, "common");
    args->addSourceFiles("common/web-common");
    args->addTarget(Visibility::Public, "runtime");
}

// ply instantiate web-documentation
void inst_webDocumentation(TargetInstantiatorArgs* args) {
    args->addIncludeDir(Visibility::Public, "documentation");
    args->addSourceFiles("documentation/web-documentation");
    args->addTarget(Visibility::Public, "reflect");
}

// ply instantiate web-cook-docs
void inst_webCookDocs(TargetInstantiatorArgs* args) {
    args->addIncludeDir(Visibility::Public, "cook-docs");
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
}

// ply instantiate web-serve-docs
void inst_webServeDocs(TargetInstantiatorArgs* args) {
    args->addIncludeDir(Visibility::Public, "serve-docs");
    args->addSourceFiles("serve-docs/ply-web-serve-docs");
    args->addTarget(Visibility::Public, "runtime");
    args->addTarget(Visibility::Private, "pylon-reflect");
    args->addTarget(Visibility::Private, "web-documentation");
    args->addTarget(Visibility::Private, "web-common"); // for ResponseIface
}

// ply extern plywood.libsass.macports
ExternResult extern_libsass_macports(ExternCommand cmd, ExternProviderArgs* args) {
    if (args->providerArgs) {
        return {ExternResult::BadArgs, ""};
    }
    PackageManager* pkgMan = HostTools::get()->getMacPorts();
    if (!pkgMan) {
        return {ExternResult::MissingPackageManager, {}};
    }

    ExternResult er = {pkgMan->isPackageInstalled("libsass")
                           ? ExternResult::Installed
                           : ExternResult::SupportedButNotInstalled,
                       ""};
    if (cmd == ExternCommand::Status) {
        return er;
    } else if (cmd == ExternCommand::Install) {
        if (er.code == ExternResult::Installed) {
            return er;
        }
        return {pkgMan->installPackage("libsass") ? ExternResult::Installed
                                                   : ExternResult::InstallFailed,
                ""};
    } else if (cmd == ExternCommand::Instantiate) {
        if (er.code != ExternResult::Installed) {
            return er;
        }
        args->dep->includeDirs.append("/opt/local/include");
        args->dep->libs.append("/opt/local/lib/libsass.a");
        return {ExternResult::Instantiated, ""};
    }
    PLY_ASSERT(0);
    return {ExternResult::Unknown, ""};
}

// ply extern plywood.libsass.apt
ExternResult extern_libsass_apt(ExternCommand cmd, ExternProviderArgs* args) {
    if (args->providerArgs) {
        return {ExternResult::BadArgs, ""};
    }
    PackageManager* pkgMan = HostTools::get()->getApt();
    if (!pkgMan) {
        return {ExternResult::MissingPackageManager, {}};
    }

    ExternResult er = {pkgMan->isPackageInstalled("libsass-dev")
                           ? ExternResult::Installed
                           : ExternResult::SupportedButNotInstalled,
                       ""};
    if (cmd == ExternCommand::Status) {
        return er;
    } else if (cmd == ExternCommand::Install) {
        if (er.code == ExternResult::Installed) {
            return er;
        }
        return {pkgMan->installPackage("libsass-dev") ? ExternResult::Installed
                                                   : ExternResult::InstallFailed,
                ""};
    } else if (cmd == ExternCommand::Instantiate) {
        if (er.code != ExternResult::Installed) {
            return er;
        }
        args->dep->libs.append("-lsass");
        return {ExternResult::Instantiated, ""};
    }
    PLY_ASSERT(0);
    return {ExternResult::Unknown, ""};
}

// ply extern plywood.libsass.prebuilt
ExternResult extern_libsass_prebuilt(ExternCommand cmd, ExternProviderArgs* args) {
    // Toolchain filters
    if (args->toolchain->targetPlatform.name != "windows") {
        return {ExternResult::UnsupportedToolchain, "Target platform must be 'windows'"};
    }
    if (findItem(ArrayView<const StringView>{"x86", "x64"}, args->toolchain->arch) < 0) {
        return {ExternResult::UnsupportedToolchain, "Target arch must be 'x86' or 'x64'"};
    }
    if (args->providerArgs) {
        return {ExternResult::BadArgs, ""};
    }

    StringView version = "3.6.3";
    String archiveName = String::format("libsass-{}-{}", version, args->toolchain->arch);

    // Handle Command
    Tuple<ExternResult, ExternFolder*> er = args->findExistingExternFolder(args->toolchain->arch);
    if (cmd == ExternCommand::Status) {
        return er.first;
    } else if (cmd == ExternCommand::Install) {
        if (er.first.code != ExternResult::SupportedButNotInstalled) {
            return er.first;
        }
        ExternFolder* externFolder = args->createExternFolder(args->toolchain->arch);
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
